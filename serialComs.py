# Imports
import serial
from serial.tools import list_ports
import time

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
            # Wait a moment for the PIC's USB to initialize before we continue
            time.sleep(1)  # << Important for stability
            return
        else:
            # Case the auto-connection fails (No device found)
            print(f"\n⚠️ No device ('{DeviceRecherched}') found for auto-connect.")

    except serial.SerialException as e:
        # Case the auto-connection fails (Port error)
        print(f"\n❌ Auto-Connection Failed: {e}.")

    # --- Step 2: Manual Selection or Skip ---
    availablePorts = list_available_ports()  # Get the list of all the available ports

    if not availablePorts:
        print("\n❌ No Ports detected. Game will be running without the PIC.")
        ser = None
        return

    print("\n--- Manual Port Selection ---")
    for i, (device, description) in enumerate(availablePorts):
        print(f"[{i + 1}]: {device} ({description})")

    while True:
        prompt = f"\nEnter number (1-{len(availablePorts)}) or press [Enter] to play with keyboard: "
        userChoice = input(prompt).strip()

        if userChoice == "":
            print("\n👍 Skipping manual setup. Game will run without PIC controller.")
            ser = None
            return

        try:
            index = int(userChoice) - 1
            if 0 <= index < len(availablePorts):
                selected_dev = availablePorts[index][0]
                ser = serial.Serial(selected_dev, 9600, timeout=0.01)
                print(f"\n✅ Manually Connected to {availablePorts[index][1]}.")
                time.sleep(1)  # << Important for stability
                return
            else:
                print("Invalid number. Please try again.")
        except ValueError:
            print("Invalid input. Please enter a number or press Enter.")
        except serial.SerialException as e:
            print(f"❌ Could not open port {selected_dev}: {e}. Try another port or press Enter to skip.")
            ser = None


def read_serial_input():
    """Reads messages from the PIC if available. Returns one line."""
    if ser and ser.in_waiting > 0:
        line = ser.readline().decode(errors='ignore').strip()
        # Only return non-empty lines
        if line:
            return line
    return None


def close_serial():
    """Closes the serial port cleanly."""
    if ser:
        ser.close()
        print("Serial port closed.")


# ===================================================================
# --- NEW PROTOCOL SENDER FUNCTIONS ---
# ===================================================================

def _send_command(command_str):
    """
    Private helper function to send a formatted command string to the PIC.
    Ensures the message ends with \r\n (as \n is often used by Python).
    """
    if ser:
        try:
            # Ensure command ends with a single \r\n
            message = command_str.strip() + '\r\n'
            ser.write(message.encode('ascii'))
            # print(f"PC -> PIC: {message.strip()}") # Uncomment for debugging
        except serial.SerialException as e:
            print(f"Error writing to serial: {e}")


def send_live_score(score):
    """CC:S,<n> - Send the live score to the PIC for display."""
    _send_command(f"CC:S,{score}")


def send_game_over():
    """CC:GO,1 - Tell the PIC the game is over and to check/save the best score."""
    _send_command("CC:GO,1")


def send_select_slot(slot_id):
    """CC:SEL,<id> - Tell the PIC to change the active score slot."""
    if 0 <= slot_id <= 3:
        _send_command(f"CC:SEL,{slot_id}")


def send_request_best():
    """CC:RB - Ask the PIC to send its current best score for the active slot."""
    _send_command("CC:RB")


def send_angle(angle):
    """CC:A,<deg> - Send an angle (0-180) to the PIC."""
    _send_command(f"CC:A,{angle}")


def send_ping():
    """CC:PING - Send a ping to see if the card is responsive."""
    _send_command("CC:PING")

def send_mode_select(mode_id):
    """CC:MODE,<id> - Tell the PIC which input mode to use."""
    _send_command(f"CC:MODE,{mode_id}")


def send_calibrate():
    """CC:CAL - Tell the PIC to run its calibration routine."""
    _send_command("CC:CAL")