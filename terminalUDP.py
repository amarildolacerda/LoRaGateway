import socket
import argparse
import threading
import time
import re
from colorama import init, Fore

init(autoreset=True)

# Configurações
BROADCAST_PORT = 1234  # Porta UDP para escutar broadcasts (deve corresponder ao _localPort do RadioUDP)
SEND_INTERVAL = 5      # Intervalo entre envios de status
message_id = 0
state = "on"

def format_message(message):
    """Formata a mensagem para exibição (hex e ASCII)."""
    hex_rep = ' '.join(f'{byte:02X}' for byte in message)
    ascii_rep = ''.join(chr(byte) if 32 <= byte <= 126 else '.' for byte in message)
    return f'[{hex_rep}] -> {ascii_rep}'

def listen_for_messages(udp_socket, term):
    """Escuta mensagens UDP e responde conforme o protocolo."""
    global message_id
    
    print(Fore.CYAN + f"Escutando mensagens UDP na porta {BROADCAST_PORT}...")
    
    while True:
        try:
            data, addr = udp_socket.recvfrom(1024)
            if not data:
                continue

            # Decodifica o cabeçalho (5 bytes)
            if len(data) < 5:
                print(Fore.RED + f"Mensagem muito curta: {len(data)} bytes")
                continue

            to, from_, id_, length, hop = data[:5]
            payload = data[5:].decode().strip()
            if (from_ == term): continue

            print(Fore.YELLOW + f"Recebido de {addr} -> To: {to}, From: {from_}, ID: {id_}, Payload: {payload}")
            print(Fore.YELLOW + f"Raw: {format_message(data)}")

            # Responde conforme o payload
            message_id = (message_id + 1) % 256
            if "ping" in payload:
                response = bytes([from_, term, message_id, length, hop-1]) + b"{pong|ok}\n"
            elif "pub" in payload:
                response = bytes([from_, term, message_id, length, hop-1]) + b"{pub|ok}\n"
            elif "ack" in payload:
                continue   
            else:
                response = bytes([from_, term, message_id, length, hop-1]) + b"{ack|ok}\n"

            udp_socket.sendto(response, addr)
            print(Fore.MAGENTA + f"Enviado -> {format_message(response)}")

        except Exception as e:
            print(Fore.RED + f"Erro ao processar mensagem: {e}")

def send_periodic_status(udp_socket, term, server_addr=None):
    """Envia {st|on}/{st|off} periodicamente."""
    global message_id, state
    
    while True:
        try:
            message_id = (message_id + 1) % 256
            state = "off" if state == "on" else "on"
            
            # Monta a mensagem no formato esperado pelo RadioUDP
            response = bytes([0x00, term, message_id, 0x05, 0x03]) + f"{{st|{state}}}\n".encode()
            
            # Se tiver um endereço específico, envia para lá, senão faz broadcast
            if server_addr:
                udp_socket.sendto(response, server_addr)
            else:
                udp_socket.sendto(response, ('<broadcast>', BROADCAST_PORT))
            
            print(Fore.BLUE + f"Enviado -> {format_message(response)}")
            time.sleep(SEND_INTERVAL)
            
        except Exception as e:
            print(Fore.RED + f"Erro ao enviar status: {e}")
            time.sleep(1)

def run_udp_client(term, server_ip=None):
    """Configura e executa o cliente UDP."""
    try:
        # Configura o socket UDP
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.bind(('0.0.0.0', BROADCAST_PORT))
        
        server_addr = (server_ip, BROADCAST_PORT) if server_ip else None
        
        # Inicia thread para envio periódico
        threading.Thread(
            target=send_periodic_status, 
            args=(sock, term, server_addr),
            daemon=True
        ).start()
        
        # Escuta mensagens na thread principal
        listen_for_messages(sock, term)
        
    except Exception as e:
        print(Fore.RED + f"Erro no cliente UDP: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Cliente UDP para comunicação com RadioUDP.")
    parser.add_argument("term", type=int, help="Número do terminal (ex: 10, 11, etc.)")
    parser.add_argument("--server", type=str, help="Endereço IP do servidor (opcional)", default=None)
    args = parser.parse_args()
    
    run_udp_client(args.term, args.server)