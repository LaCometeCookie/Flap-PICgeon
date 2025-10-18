import serial
import serial.tools.list_ports
import time


def select_com_port(prompt_message):
    """
    Lists available COM ports and asks the user to select one.
    """
    # We filter out COM1 as it's often a built-in system port
    ports = [p for p in serial.tools.list_ports.comports() if "COM1" not in p.device]
    print(f"\n--- {prompt_message} ---")
    if not ports:
        print("Aucun port COM disponible n'a été trouvé !")
        return None

    for i, port in enumerate(ports):
        print(f"  {i}: {port.device} - {port.description}")

    while True:
        try:
            choice = int(input("Veuillez choisir un port (entrez le numéro) : "))
            if 0 <= choice < len(ports):
                return ports[choice].device
            else:
                print("Choix invalide, veuillez réessayer.")
        except (ValueError, IndexError):
            print("Entrée invalide. Veuillez entrer un numéro de la liste.")


def main():
    """
    Main function to connect and listen to the serial port.
    """
    port_name = select_com_port("Sélectionnez le port de la carte PIC à écouter")

    if not port_name:
        print("Aucun port sélectionné. Programme arrêté.")
        return

    try:
        # 'with' statement handles opening and closing the port automatically
        with serial.Serial(port_name, baudrate=9600, timeout=1) as ser:
            print(f"\n--- Connexion établie sur {ser.name} ---")
            print("--- Écoute des données... (Appuyez sur Ctrl+C pour arrêter) ---")

            while True:
                # Read one full line from the serial port, ending in '\n'
                line = ser.readline()

                if line:  # If 'line' is not empty (i.e., data was received)
                    try:
                        # Decode the bytes into a readable string and remove whitespace
                        received_data = line.decode('utf-8').strip()
                        print(f"Reçu: '{received_data}'")
                    except UnicodeDecodeError:
                        # If data isn't valid text, print the raw bytes
                        print(f"Reçu (données brutes): {line}")

    except serial.SerialException as e:
        print(f"\nERREUR de port série: {e}")
        print("Vérifiez que la carte est connectée et que le port n'est pas utilisé par un autre programme.")
    except KeyboardInterrupt:
        print("\n--- Programme arrêté par l'utilisateur ---")
    finally:
        print("--- Connexion fermée ---")


# Run the main function when the script is executed
if __name__ == "__main__":
    main()