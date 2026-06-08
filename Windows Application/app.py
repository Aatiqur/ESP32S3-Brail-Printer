from flask import Flask, request, jsonify
import serial
import serial.tools.list_ports
import time

app = Flask(__name__)
arduino = None

# --- Hardware Constants ---
CMD_PRINT = 'P'
CMD_DOT_SHIFT_X = 'X'
CMD_CELL_SHIFT = 'C'
CMD_Y_SHIFT_DOT_ROW = 'Y'
CMD_LINE_FEED = 'L'
CMD_GO_HOME = 'H'

MAX_CELLS_PER_ROW = 20
MAX_BRAILLE_ROWS = 25

# --- Braille Logic Maps ---
PREFIX_JUKTO_2 = '⠲'
PREFIX_JUKTO_3_PLUS = '⠶'
POETRY_SIGN = '⠚'
NUMBER_SIGN = '⠼'

VOWEL_MAP = {
    'অ': '⠁', 'আ': '⠡', 'ই': '⠊', 'ঈ': '⠔', 'উ': '⠥', 'ঊ': '⠳', 'ঋ': '⠗',
    'এ': '⠑', 'ঐ': '⠕', 'ও': '⠕', 'ঔ': '⠪'
}
CONSONANT_MAP = {
    'ক': '⠅', 'খ': '⠭', 'গ': '⠛', 'ঘ': '⠫', 'ঙ': '⠜',
    'চ': '⠉', 'ছ': '⠡', 'জ': '⠚', 'ঝ': '⠵', 'ঞ': '⠒',
    'ট': '⠾', 'ঠ': '⠺', 'ড': '⠙', 'ঢ': '⠮', 'ণ': '⠼',
    'ত': '⠞', 'থ': '⠹', 'দ': '⠙', 'ধ': '⠮', 'ন': '⠝',
    'প': '⠏', 'ফ': '⠋', 'ব': '⠃', 'ভ': '⠧', 'ম': '⠍',
    'য': '⠽', 'র': '⠗', 'ল': '⠇', 'শ': '⠩', 'ষ': '⠯',
    'স': '⠎', 'হ': '⠓', 'ড়': '⠤', 'ঢ়': '⠤', 'য়': '⠽',
    'ৎ': '⠠', 'ক্ষ': '⠟', 'জ্ঞ': '⠱'
}
KAR_MAP = {
    'া': '⠡', 'ি': '⠊', 'ী': '⠔', 'ু': '⠥', 'ূ': '⠳',
    'ৃ': '⠗', 'ে': '⠑', 'ৈ': '⠕', 'ো': '⠕', 'ৌ': '⠪'
}
ENGLISH_MAP = {
    'a': '⠁', 'b': '⠃', 'c': '⠉', 'd': '⠙', 'e': '⠑', 'f': '⠋', 'g': '⠛', 'h': '⠓', 'i': '⠊', 'j': '⠚',
    'k': '⠅', 'l': '⠇', 'm': '⠍', 'n': '⠝', 'o': '⠕', 'p': '⠏', 'q': '⠟', 'r': '⠗', 's': '⠎', 't': '⠞',
    'u': '⠥', 'v': '⠧', 'w': '⠺', 'x': '⠭', 'y': '⠽', 'z': '⠵',
    'A': '⠨⠁', 'B': '⠨⠃', 'C': '⠨⠉', 'D': '⠨⠙', 'E': '⠨⠑', 'F': '⠨⠋', 'G': '⠨⠛', 'H': '⠨⠓', 'I': '⠨⠊', 'J': '⠨⠚',
    'K': '⠨⠅', 'L': '⠨⠇', 'M': '⠨⠍', 'N': '⠨⠝', 'O': '⠨⠕', 'P': '⠨⠏', 'Q': '⠨⠟', 'R': '⠨⠗', 'S': '⠨⠎', 'T': '⠨⠞',
    'U': '⠨⠥', 'V': '⠨⠧', 'W': '⠨⠺', 'X': '⠨⠭', 'Y': '⠨⠽', 'Z': '⠨⠵'
}
OTHER_MAP = {
    '্': '', '': '',  # Halant (no representation)
    'ং': '⠰', 'ঃ': '⠆', 'ঁ': '⠈', ' ': ' ',
    '.': '⠲', ',': '⠂', '?': '⠦', '!': '⠖', ';': '⠆', ':': '⠒',
    '-': '⠤', '(': '⠶', ')': '⠶', '"': '⠦', "'": '⠄'
}
DIGIT_MAP = {
    '1': '⠁', '2': '⠃', '3': '⠉', '4': '⠙', '5': '⠑',
    '6': '⠋', '7': '⠛', '8': '⠓', '9': '⠊', '0': '⠚',
    '১': '⠁', '২': '⠃', '৩': '⠉', '৪': '⠙', '৫': '⠑',
    '৬': '⠋', '৭': '⠛', '৮': '⠓', '৯': '⠊', '০': '⠚'
}


def translate_word_to_cells(word, is_number):
    cells = []
    i = 0
    while i < len(word):
        char = word[i]
        if char in DIGIT_MAP:
            if not is_number: cells.append(NUMBER_SIGN)
            is_number = True
            cells.append(DIGIT_MAP[char])
            i += 1
            continue
        else:
            is_number = False

        if char in CONSONANT_MAP and i + 1 < len(word) and word[i + 1] == '্':
            conjunct = [CONSONANT_MAP[char]]
            j = i + 2
            while j < len(word):
                if j < len(word) and word[j] in CONSONANT_MAP:
                    conjunct.append(CONSONANT_MAP[word[j]])
                    j += 1
                    if j < len(word) and word[j] == '্':
                        j += 1
                    else:
                        break
                else:
                    break
            num = len(conjunct)
            if num >= 3:
                cells.append(PREFIX_JUKTO_3_PLUS)
            elif num == 2:
                cells.append(PREFIX_JUKTO_2)
            cells.extend(conjunct)
            if j < len(word) and word[j] in KAR_MAP:
                cells.append(KAR_MAP[word[j]])
                i = j + 1
            else:
                i = j
            continue

        # Check for kar following a consonant (e.g., ন + ো = নো)
        if char in CONSONANT_MAP and i + 1 < len(word) and word[i + 1] in KAR_MAP:
            cells.append(CONSONANT_MAP[char])
            kar_char = word[i + 1]
            cells.append(KAR_MAP[kar_char])
            i += 2
            continue

        # Handle vowels
        if char in VOWEL_MAP:
            cells.append(VOWEL_MAP[char])
            i += 1
            continue
        
        # Handle consonants without kar (add inherent vowel)
        if char in CONSONANT_MAP:
            cells.append(CONSONANT_MAP[char])
            # Check if next char is a vowel (should not add inherent vowel)
            if i + 1 < len(word) and word[i + 1] in VOWEL_MAP: 
                cells.append(VOWEL_MAP['অ'])
            i += 1
            continue
        
        # Handle English characters
        if char in ENGLISH_MAP:
            cells.append(ENGLISH_MAP[char])
            i += 1
            continue
        
        # Handle other characters
        if char in OTHER_MAP:
            cells.append(OTHER_MAP[char])
            i += 1
            continue
        
        # Handle unknown characters
        cells.append(' ')
        i += 1
        
    return cells, is_number


def build_single_line_command(line_text, is_poetry=False):
    """Build command for ONE line"""
    words = line_text.split(' ')
    final_line_cells = []
    is_number = False

    for word in words:
        if not word:
            final_line_cells.append(' ')
            continue

        cells, is_number = translate_word_to_cells(word, is_number)
        for cell_group in cells:
            # cell_group is already a complete Braille character string, add it as-is
            if len(final_line_cells) < MAX_CELLS_PER_ROW: 
                final_line_cells.append(cell_group)
        if len(final_line_cells) < MAX_CELLS_PER_ROW: 
            final_line_cells.append(' ')

    if is_poetry and final_line_cells and len(final_line_cells) < MAX_CELLS_PER_ROW:
        final_line_cells.append(POETRY_SIGN)

    while len(final_line_cells) < MAX_CELLS_PER_ROW:
        final_line_cells.append(' ')

    final_line_cells.reverse()

    final_command = CMD_GO_HOME
    flat_cells = []
    
    for char in final_line_cells:
        if char == ' ':
            flat_cells.append('000000')
            continue
        try:
            code = ord(char) - 0x2800
            if code < 0 or code > 63:
                flat_cells.append('000000')
            else:
                raw_bits = [
                    (code >> 0) & 1, (code >> 1) & 1, (code >> 2) & 1,
                    (code >> 3) & 1, (code >> 4) & 1, (code >> 5) & 1
                ]
                mirrored_bits = [
                    raw_bits[3], raw_bits[4], raw_bits[5],
                    raw_bits[0], raw_bits[1], raw_bits[2]
                ]
                flat_cells.append("".join(map(str, mirrored_bits)))
        except:
            flat_cells.append('000000')

    # PASS 1
    for i, binary in enumerate(flat_cells):
        if binary[0] == '1': final_command += CMD_PRINT
        final_command += CMD_DOT_SHIFT_X
        if binary[3] == '1': final_command += CMD_PRINT
        final_command += CMD_DOT_SHIFT_X
        if i < MAX_CELLS_PER_ROW - 1: final_command += CMD_CELL_SHIFT

    final_command += CMD_Y_SHIFT_DOT_ROW
    final_command += CMD_GO_HOME

    # PASS 2
    for i, binary in enumerate(flat_cells):
        if binary[1] == '1': final_command += CMD_PRINT
        final_command += CMD_DOT_SHIFT_X
        if binary[4] == '1': final_command += CMD_PRINT
        final_command += CMD_DOT_SHIFT_X
        if i < MAX_CELLS_PER_ROW - 1: final_command += CMD_CELL_SHIFT

    final_command += CMD_Y_SHIFT_DOT_ROW
    final_command += CMD_GO_HOME

    # PASS 3
    for i, binary in enumerate(flat_cells):
        if binary[2] == '1': final_command += CMD_PRINT
        final_command += CMD_DOT_SHIFT_X
        if binary[5] == '1': final_command += CMD_PRINT
        final_command += CMD_DOT_SHIFT_X
        if i < MAX_CELLS_PER_ROW - 1: final_command += CMD_CELL_SHIFT

    final_command += CMD_Y_SHIFT_DOT_ROW
    final_command += CMD_GO_HOME
    final_command += CMD_LINE_FEED
    
    return final_command


def find_arduino_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "Arduino" in port.description or "CH340" in port.description:
            return port.device
    return None


def connect_arduino():
    global arduino
    port = find_arduino_port()
    if port:
        try:
            arduino = serial.Serial(port, 115200, timeout=2)
            time.sleep(2)
            print(f"✅ Connected to Arduino on {port}")
        except Exception as e:
            print(f"❌ Connection error: {e}")
            arduino = None
    else:
        print("❌ Arduino not found")


def send_command(command):
    """Send command and wait for response"""
    global arduino
    if not arduino: 
        connect_arduino()
    if not arduino: 
        return "❌ Offline"
    
    try:
        # Clear buffers
        arduino.reset_input_buffer()
        arduino.reset_output_buffer()
        time.sleep(0.05)
        
        # Send command
        print(f"[SEND] Command length: {len(command)}")
        arduino.write((command + '\n').encode())
        arduino.flush()
        
        # Wait for response - FIXED VERSION
        timeout = 120
        start = time.time()
        buffer = ""
        
        while time.time() - start < timeout:
            if arduino.in_waiting > 0:
                try:
                    data = arduino.read(arduino.in_waiting).decode(errors='ignore')
                    buffer += data
                    
                    if data.strip():
                        print(f"[RECV] {data.strip()}")
                    
                    # Check for completion
                    if "DONE" in buffer or "LINE_COMPLETE" in buffer or "OK" in buffer:
                        print(f"[SUCCESS] Received completion signal")
                        return "✅"
                    
                except:
                    pass
            
            time.sleep(0.05)  # Check every 50ms
        
        print(f"[TIMEOUT] No response after {timeout}s")
        return "⚠️ Timeout"
        
    except Exception as e:
        print(f"[ERROR] {e}")
        return f"❌ {e}"


@app.route('/')
def home():
    try:
        with open("index.html", encoding="utf-8") as f:
            return f.read()
    except:
        return "Missing index.html"


@app.route('/print-lines', methods=['POST'])
def print_lines():
    """Print multiple lines"""
    data = request.get_json()
    text = data.get('text', '')
    is_poetry = data.get('is_poetry', False)
    
    # Split by newlines - preserve all line breaks
    lines = text.splitlines()
    
    print(f"\n{'='*60}")
    print(f"📄 PRINTING {len(lines)} LINES")
    print(f"{'='*60}")
    print(f"Raw text received:")
    print(repr(text))
    print()
    
    results = []
    
    for idx, line_text in enumerate(lines):
        print(f"[LINE {idx+1}/{len(lines)}]")
        print(f"Text: '{line_text}'")
        
        if line_text.strip():
            cmd = build_single_line_command(line_text.strip(), is_poetry)
            print(f"Command: {len(cmd)} chars")
            msg = send_command(cmd)
            results.append({"line": line_text, "status": msg})
            print(f"Result: {msg}\n")
            time.sleep(1.0)
        else:
            print("Empty line - line feed only")
            msg = send_command(CMD_LINE_FEED + CMD_GO_HOME)
            results.append({"line": "(empty)", "status": msg})
            print(f"Result: {msg}\n")
            time.sleep(0.5)
    
    print(f"{'='*60}")
    print(f"✅ COMPLETED")
    print(f"{'='*60}\n")
    
    return jsonify({"status": "success", "results": results})


if __name__ == '__main__':
    connect_arduino()
    app.run(port=5000, debug=False)