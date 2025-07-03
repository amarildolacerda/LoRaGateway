import socket
import argparse
import threading
import time
import re
from colorama import init, Fore

init(autoreset=True)

# Configurações padrão (serão sobrescritas pelo broadcast)
SERVER_IP = None  # Será definido pelo broadcast
SERVER_PORT = None  # Será definido pelo broadcast
SEND_INTERVAL = 5  # Intervalo entre envios de {st|on}
BROADCAST_PORT = 12346  # Porta para escutar o broadcast
message_id = 0
state = "on"

def format_message(message):
    hex_rep = ' '.join(f'{byte:02X}' for byte in message)
    ascii_rep = ''.join(chr(byte) if 32 <= byte <= 126 else '.' for byte in message)
    return f'[{hex_rep}] -> {ascii_rep}'

def listen_for_broadcast():
    global SERVER_IP, SERVER_PORT
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('0.0.0.0', BROADCAST_PORT))
    print(Fore.CYAN + f"Escutando broadcasts UDP na porta {BROADCAST_PORT}...")

    while True:
        try:
            data, addr = sock.recvfrom(1024)
            message = data.decode().strip()
            print(Fore.YELLOW + f"Recebido (raw): {message}")  # Log para debug

            # Aceita ambos os formatos: "server-ip=" ou "ip="
            match = re.match(r"ESP_DISCOVERY\|(?:server-ip|ip)=([\d.]+)\|port=(\d+)", message)
            if match:
                SERVER_IP = match.group(1)
                SERVER_PORT = int(match.group(2))
                print(Fore.GREEN + f"Servidor descoberto: {SERVER_IP}:{SERVER_PORT}")
        except Exception as e:
            print(Fore.RED + f"Erro ao receber broadcast: {e}")


def send_periodic_status(client_socket, term):
    """Envia {st|on}/{st|off} periodicamente."""
    global message_id, state
    while True:
        if SERVER_IP is None:
            time.sleep(1)
            continue  # Espera até descobrir o servidor

        try:
            message_id = (message_id + 1) % 256
            state = "off" if state == "on" else "on"
            response = bytes([0x00, term, message_id, 0x05, 0x03]) + f"{{st|{state}}}\n".encode()
            client_socket.sendall(response)
            print(Fore.BLUE + f"Enviado -> {format_message(response)}")
            time.sleep(SEND_INTERVAL)
        except Exception as e:
            print(Fore.RED + f"Erro ao enviar: {e}")
            break

def run_client(term):
    """Thread principal: conecta ao servidor e gerencia comunicação."""
    global message_id

    # Inicia thread para escutar broadcasts
    threading.Thread(target=listen_for_broadcast, daemon=True).start()

    while SERVER_IP is None:
        time.sleep(0.5)  # Espera até receber o broadcast

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((SERVER_IP, SERVER_PORT))
            print(Fore.GREEN + f"Conectado a {SERVER_IP}:{SERVER_PORT}")

            # Inicia thread de envio periódico
            threading.Thread(target=send_periodic_status, args=(s, term), daemon=True).start()

            while True:
                data = s.recv(1024)
                if not data:
                    break

                to, from_, id_, length, hop = data[:5]
                payload = data[5:-1].decode()
                print(Fore.YELLOW + f"Recebido -> To: {to}, From: {from_}, ID: {id_}, Payload: {payload}")

                # Responde conforme o payload
                message_id = (message_id + 1) % 256
                if "ping" in payload:
                    response = bytes([from_, term, message_id, length, 3]) + b"{pong|ok}\n"
                elif "pub" in payload:
                    response = bytes([from_, term, message_id, length, 3]) + b"{pub|ok}\n"
                else:
                    response = bytes([from_, term, message_id, length, 3]) + b"{ack|ok}\n"

                s.sendall(response)
                print(Fore.MAGENTA + f"Enviado -> {format_message(response)}")

    except Exception as e:
        print(Fore.RED + f"Erro na conexão: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Cliente IoT que descobre servidor via broadcast.")
    parser.add_argument("term", type=int, help="Número do terminal (ex: 10, 11, etc.)")
    args = parser.parse_args()
    run_client(args.term)