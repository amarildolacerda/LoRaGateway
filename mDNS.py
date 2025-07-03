import socket
from zeroconf import Zeroconf, ServiceInfo

# Configuração da rede
SSID = "your-ssid"
PASSWORD = "your-password"
SERVICE_TYPE = "_http._tcp.local."
SERVICE_NAME = "homewareio._http._tcp.local."
PORT = 12345

# Criar um servidor HTTP simples
def start_http_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("0.0.0.0", PORT))
    server_socket.listen(5)

    print(f"Servidor HTTP iniciado em porta {PORT}")
    try:
      while True:
        client_socket, addr = server_socket.accept()
        request = client_socket.recv(1024)
        print(f"Requisição recebida de {addr}: ")
#{request.splitlines()[0]}
        if request.startswith("GET / "):
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            response += f"<html>Hello from ESP8266 at {socket.gethostbyname(socket.gethostname())}</html>\r\n"
        else:
            response = "HTTP/1.1 404 Not Found\r\n\r\n"

        client_socket.sendall(response.encode("utf-8"))
        client_socket.close()
    except KeyboardInterrupt:
        print("\nFinalizando...")
        zeroconf_instance.close()
# Publicar serviço via mDNS
def start_mdns_service():
    zeroconf = Zeroconf()

    service_info = ServiceInfo(
        SERVICE_TYPE,
        SERVICE_NAME,
        addresses=[socket.inet_aton(socket.gethostbyname(socket.gethostname()))],
        port=PORT,
        properties={"path": "/"},
    )

    zeroconf.register_service(service_info)
    print("Serviço mDNS registrado como {SERVICE_NAME}")
    return zeroconf

if __name__ == "__main__":
    zeroconf_instance = start_mdns_service()
    try:
        start_http_server()
        print("Escutando requisições mDNS... Pressione Ctrl+C para sair.")
        import time
        while true:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nFinalizando...")
        zeroconf_instance.close()