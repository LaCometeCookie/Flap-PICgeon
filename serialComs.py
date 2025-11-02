# Imports
import serial
from serial.tools import list_ports

# --- Global Variables ---
ser = None
DeviceRecherched = "périphérique série usb"  # Name of the name researched port


# --- Functions ---
def list_available_ports():
    """Returns a list for all available ports."""
    return [(p.device, p.description) for p in list_ports.comports()]


def connect_to_serial_port():
    """
    Tries to auto-connect. If it fails, it asks the user to
    manually select a port or skip to play with the keyboard.
    """
    global ser

    print("\n" + "=" * 40)
    print("      PIC Game Controller Setup")
    print("=" * 40)

    # --- Step 1: Try Automatic Detection ---
    try:
        matches = [p for p in list_ports.comports() if DeviceRecherched in (p.description or "").lower()]

        if matches:
            # Case the auto-connection works
            dev = matches[0].device
            ser = serial.Serial(dev, 9600, timeout=0.01)
            print(f"\n✅ Auto-Connection Successful: {dev} ({matches[0].description}).")
            return
        else:
            # Case the auto-connection fails (No device found)
            print(f"\n⚠️ No device ('{DeviceRecherched}') found for auto-connect.")

    except serial.SerialException as e:
        # Case the auto-connection fails (Port error)
        print(f"\n❌ Auto-Connection Failed: {e}.")

    # --- Step 2: Manual Selection or Skip ---
    # This part runs if auto-detection failed

    availablePorts = list_available_ports()  # Get the list of all the available ports

    # Case no port available
    if not availablePorts:
        print("\n❌ No Ports detected. Game will be running without the PIC.")
        ser = None
        return

    # Case at least one port is available
    print("\n--- Manual Port Selection ---")
    for i, (device, description) in enumerate(availablePorts):
        print(f"[{i + 1}]: {device} ({description})")

    while True:
        # This is the new, simplified prompt
        prompt = f"\nEnter number (1-{len(availablePorts)}) or press [Enter] to play with keyboard: "
        userChoice = input(prompt).strip()

        if userChoice == "":
            # User pressed Enter. This is the "skip" (play with space/keyboard)
            print("\n👍 Skipping manual setup. Game will run without PIC controller.")
            ser = None
            return

        # If not "", try to parse it as a number.
        try:
            index = int(userChoice) - 1
            if 0 <= index < len(availablePorts):
                selected_dev = availablePorts[index][0]
                ser = serial.Serial(selected_dev, 9600, timeout=0.01)
                print(f"\n✅ Manually Connected to {availablePorts[index][1]}.")
                return
            else:
                print("Invalid number. Please try again.")
        except ValueError:
            print("Invalid input. Please enter a number or press Enter.")
        except serial.SerialException as e:
            # This allows them to try another port if one fails
            print(f"❌ Could not open port {selected_dev}: {e}. Try another port or press Enter to skip.")
            ser = None


def read_serial_input():
    """Reads messages from the PIC if available."""
    if ser and ser.in_waiting > 0:
        line = ser.readline().decode(errors='ignore').strip()
        return line
    return None


def sendToPic(current_score):
    """
    Sends the score to the serial port as an ASCII string,
    followed by a carriage return to un-block getsUSBUSART().
    """
    if ser:
        try:
            score_str = str(current_score)
            message_str = score_str + '\r'
            message_bytes = message_str.encode('ascii')
            ser.write(message_bytes)
            print(f"Sent message to PIC: {message_bytes}")
        except serial.SerialException as e:
            print(f"Error writing to serial: {e}")


def close_serial():
    """Closes the serial port cleanly."""
    if ser:
        ser.close()
        print("Serial port closed.")