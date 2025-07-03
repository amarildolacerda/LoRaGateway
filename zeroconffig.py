from zeroconf import Zeroconf, ServiceBrowser

class MDNSListener:
    def update_service(self, zeroconf, type, name):
        print(f"Serviço atualizado: {name}")

    def add_service(self, zeroconf, type, name):
        print(f"Serviço encontrado: {type} {name}")

    def remove_service(self, zeroconf, type, name):
        print(f"Serviço removido: {name}")

zeroconf = Zeroconf()
listener = MDNSListener()

# Escuta por serviços mDNS contendo "arduino" ou "gateway"
ServiceBrowser(zeroconf, "_services._dns-sd._udp.local.", listener)

try:
    print("Escutando requisições mDNS... Pressione Ctrl+C para sair.")
    import time
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\nFinalizando...")
    zeroconf.close()