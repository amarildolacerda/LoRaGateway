import socket
import threading
import time
from zeroconf import Zeroconf, ServiceInfo, ServiceBrowser
from colorama import init, Fore

init(autoreset=True)

HOST = "0.0.0.0"
PORT = 12345
BROADCAST_PORT = 12346
BROADCAST_INTERVAL = 15
ALIVE_INTERVAL = 30
isGateway = True  # Alternar entre servidor (True) e cliente (False)
discovered_devices = []
running = True
zeroconf = Zeroconf()
SERVICE_NAME = "homewareio.local."
SERVICE_TYPE = "_http._tcp.local."


# Servidor mDNS
def start_mdns_server():
    ip = socket.gethostbyname(socket.gethostname())
    service_info = ServiceInfo(
        SERVICE_TYPE,
        SERVICE_NAME,
        addresses=[socket.inet_aton(ip)],
        port=PORT,
        properties={"info": "ESP8266 Gateway"},
    )
    zeroconf.register_service(service_info)
    print(Fore.GREEN + f"📡 Servidor mDNS registrado: {SERVICE_NAME} ({ip}:{PORT})")


# Cliente mDNS
class MDNSListener:
    def add_service(self, zeroconf, type, name):
        print(Fore.GREEN + f"🆕 Serviço mDNS encontrado: {name}")

def find_mdns_services():
    listener = MDNSListener()
    ServiceBrowser(zeroconf, SERVICE_TYPE, listener)
    print(Fore.BLUE + "🔎 Procurando serviços mDNS na rede...")


# Envio de broadcast UDP
def send_broadcast():
    global running
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp_socket:
        udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

        while running:
            message = f"ESP_DISCOVERY|ip={socket.gethostbyname(socket.gethostname())}|port={PORT}".encode()
            udp_socket.sendto(message, ('255.255.255.255', BROADCAST_PORT))
            print(Fore.BLUE + f"🔊 Broadcast enviado: {message.decode()}")
            time.sleep(BROADCAST_INTERVAL)


# Escuta por respostas ao broadcast
def listen_for_devices():
    global discovered_devices, running
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as listen_socket:
        listen_socket.bind(('', BROADCAST_PORT))

        while running:
            data, addr = listen_socket.recvfrom(1024)
            message = data.decode(errors="ignore")

            if message.startswith("ESP_DISCOVERY|"):
                device_ip = addr[0]
                if device_ip not in discovered_devices:
                    discovered_devices.append(device_ip)
                    print(Fore.GREEN + f"🆕 Dispositivo descoberto: {device_ip}")


# Envia mensagens "alive" para os dispositivos descobertos
def send_alive_messages():
    global discovered_devices, running
    while running:
        for device in discovered_devices:
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as alive_socket:
                    message = b"ESP_DISCOVERY|alive|server\n"
                    alive_socket.sendto(message, (device, BROADCAST_PORT))
                    print(Fore.CYAN + f"📡 Enviando alive para {device}")
            except Exception as e:
                print(Fore.RED + f"Erro ao enviar alive: {e}")
        time.sleep(ALIVE_INTERVAL)


# Servidor TCP para ESP8266
def start_server():
    global running
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((HOST, PORT))
        server_socket.listen()
        print(Fore.GREEN + f"🌐 Servidor escutando em {HOST}:{PORT}")

        while running:
            try:
                conn, addr = server_socket.accept()
                with conn:
                    print(Fore.CYAN + f"🤝 Cliente conectado: {addr}")

                    while running:
                        data = conn.recv(1024)
                        if not data:
                            break

                        payload = data.decode(errors="ignore")
                        print(Fore.YELLOW + f"📥 Recebido -> {payload}")

                        if payload.startswith("ESP_DISCOVERY|"):
                            response = b"ESP_DISCOVERY|ack|server-ok\n"
                            conn.sendall(response)
                            print(Fore.MAGENTA + f"🚀 Enviado -> {response.decode(errors='ignore')}")
                        else:
                            print(Fore.RED + "⚠ Mensagem ignorada! Não pertence ao protocolo ESP_DISCOVERY.")
            except KeyboardInterrupt:
                running = False
                print(Fore.RED + "\nServidor encerrado pelo usuário.")


# Inicialização
if __name__ == "__main__":
    try:
        if isGateway:
            start_mdns_server()
            threading.Thread(target=listen_for_devices, daemon=True).start()
        else:
            find_mdns_services()
            threading.Thread(target=send_broadcast, daemon=True).start()

        threading.Thread(target=send_alive_messages, daemon=True).start()
        start_server()

    except KeyboardInterrupt:
        running = False
        zeroconf.close()
        print(Fore.RED + "\nExecução interrompida! Encerrando...")