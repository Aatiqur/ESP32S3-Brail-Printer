#!/usr/bin/env python3
"""
SINC ROBOTICS Braille Embosser - Desktop Application
Standalone desktop app with instant read-aloud functionality
"""

import sys
import threading
import pyttsx3
import serial
import serial.tools.list_ports
import time
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QTextEdit, QPushButton, QLabel, QCheckBox, QMessageBox
)
from PyQt5.QtCore import Qt, pyqtSignal, QThread
from PyQt5.QtGui import QFont

# ========== BRAILLE MAPS ==========
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
    'স': '⠎', 'হ': '⠓'
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
    '্': '', 'ং': '⠰', 'ঃ': '⠆', 'ঁ': '⠈', ' ': ' ',
    '.': '⠲', ',': '⠂', '?': '⠦', '!': '⠖', ';': '⠆', ':': '⠒',
    '-': '⠤', '(': '⠶', ')': '⠶', '"': '⠦', "'": '⠄'
}
DIGIT_MAP = {
    '1': '⠁', '2': '⠃', '3': '⠉', '4': '⠙', '5': '⠑',
    '6': '⠋', '7': '⠛', '8': '⠓', '9': '⠊', '0': '⠚'
}

# Constants
MAX_CELLS_PER_ROW = 20
MAX_LINES = 25

# ========== TRANSLATION FUNCTION ==========
def translate_to_braille(text):
    """Convert text to Braille"""
    result = ''
    i = 0
    while i < len(text):
        char = text[i]
        
        # Check for consonant + kar
        if char in CONSONANT_MAP and i + 1 < len(text) and text[i + 1] in KAR_MAP:
            result += CONSONANT_MAP[char]
            result += KAR_MAP[text[i + 1]]
            i += 2
        # Check for kar alone
        elif char in KAR_MAP:
            result += KAR_MAP[char]
            i += 1
        # Check for vowel
        elif char in VOWEL_MAP:
            result += VOWEL_MAP[char]
            i += 1
        # Check for consonant
        elif char in CONSONANT_MAP:
            result += CONSONANT_MAP[char]
            i += 1
        # Check for English
        elif char in ENGLISH_MAP:
            result += ENGLISH_MAP[char]
            i += 1
        # Check for digits
        elif char in DIGIT_MAP:
            result += DIGIT_MAP[char]
            i += 1
        # Check for other characters
        elif char in OTHER_MAP:
            result += OTHER_MAP[char]
            i += 1
        else:
            result += char
            i += 1
    
    return result

# ========== ARDUINO COMMUNICATION ==========
class ArduinoCommunication:
    """Handle Arduino communication"""
    
    def __init__(self):
        self.arduino = None
        self.port = None
        self.find_arduino()
    
    def find_arduino(self):
        """Find Arduino on available ports"""
        try:
            ports = [p.device for p in serial.tools.list_ports.comports()]
            if ports:
                self.port = ports[0]
                print(f"Found Arduino on {self.port}")
                return True
            return False
        except Exception as e:
            print(f"Error finding Arduino: {e}")
            return False
    
    def connect(self):
        """Connect to Arduino"""
        if not self.port:
            return False
        try:
            self.arduino = serial.Serial(self.port, 115200, timeout=1)
            time.sleep(2)  # Wait for Arduino to initialize
            print(f"Connected to Arduino on {self.port}")
            return True
        except Exception as e:
            print(f"Error connecting to Arduino: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from Arduino"""
        if self.arduino:
            self.arduino.close()
            self.arduino = None
    
    def send_command(self, command):
        """Send command to Arduino"""
        if not self.arduino:
            return False
        try:
            self.arduino.write(command.encode())
            self.arduino.flush()
            time.sleep(0.5)
            return True
        except Exception as e:
            print(f"Error sending command: {e}")
            return False


# ========== TEXT-TO-SPEECH THREAD ==========
class TextToSpeechThread(QThread):
    finished = pyqtSignal()
    
    def __init__(self, text):
        super().__init__()
        self.text = text
    
    def run(self):
        try:
            engine = pyttsx3.init()
            engine.setProperty('rate', 150)
            engine.setProperty('volume', 1.0)
            engine.say(self.text)
            engine.runAndWait()
        except Exception as e:
            print(f"TTS Error: {e}")
        finally:
            self.finished.emit()


# ========== MAIN APPLICATION ==========
class BrailleDesktopApp(QMainWindow):
    """Main desktop application"""
    
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SINC ROBOTICS - Braille Embosser")
        self.setGeometry(100, 100, 1000, 600)
        
        self.arduino = ArduinoCommunication()
        self.tts_thread = None
        
        self.setup_ui()
    
    def setup_ui(self):
        """Setup UI"""
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        
        layout = QHBoxLayout(main_widget)
        main_widget.setStyleSheet("background-color: #0f3460;")
        
        # ===== LEFT PANEL =====
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        left_panel.setStyleSheet("background-color: #1a1a2e; border-radius: 10px;")
        
        # Title
        title = QLabel("📝 INPUT TEXT")
        title.setFont(QFont("Arial", 11, QFont.Bold))
        title.setStyleSheet("color: #00d4ff;")
        left_layout.addWidget(title)
        
        # Text input
        self.text_input = QTextEdit()
        self.text_input.setPlaceholderText("এখানে লিখুন... / Type here...")
        self.text_input.setFont(QFont("Arial", 10))
        self.text_input.setStyleSheet("""
            QTextEdit {
                background-color: #16213e;
                color: #ffffff;
                border: 2px solid #00d4ff;
                border-radius: 5px;
                padding: 8px;
            }
        """)
        self.text_input.textChanged.connect(self.update_preview)
        left_layout.addWidget(self.text_input)
        
        # Auto read checkbox
        self.auto_read = QCheckBox("🔊 Read aloud while typing")
        self.auto_read.setStyleSheet("color: #ffffff;")
        left_layout.addWidget(self.auto_read)
        
        # Buttons
        read_btn = QPushButton("🔊 Read Aloud")
        read_btn.setFont(QFont("Arial", 9, QFont.Bold))
        read_btn.setStyleSheet("""
            QPushButton {
                background-color: #ff6b6b;
                color: white;
                border: none;
                padding: 8px;
                border-radius: 5px;
            }
            QPushButton:hover { background-color: #ff5252; }
        """)
        read_btn.clicked.connect(self.read_aloud)
        left_layout.addWidget(read_btn)
        
        print_btn = QPushButton("🖨️ SEND TO ARDUINO")
        print_btn.setFont(QFont("Arial", 9, QFont.Bold))
        print_btn.setStyleSheet("""
            QPushButton {
                background-color: #00d4ff;
                color: #000;
                border: none;
                padding: 8px;
                border-radius: 5px;
            }
            QPushButton:hover { background-color: #00b8cc; }
        """)
        print_btn.clicked.connect(self.print_to_arduino)
        left_layout.addWidget(print_btn)
        
        # Status
        self.status = QLabel("✅ Ready")
        self.status.setStyleSheet("color: #00ff00;")
        left_layout.addWidget(self.status)
        
        layout.addWidget(left_panel, 1)
        
        # ===== RIGHT PANEL =====
        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        right_panel.setStyleSheet("background-color: #1a1a2e; border-radius: 10px;")
        
        # Title
        preview_title = QLabel("👁️ BRAILLE PREVIEW")
        preview_title.setFont(QFont("Arial", 11, QFont.Bold))
        preview_title.setStyleSheet("color: #00d4ff;")
        right_layout.addWidget(preview_title)
        
        # Preview
        self.preview = QTextEdit()
        self.preview.setReadOnly(True)
        self.preview.setFont(QFont("Courier New", 14))
        self.preview.setStyleSheet("""
            QTextEdit {
                background-color: #16213e;
                color: #00ff00;
                border: 2px solid #00d4ff;
                border-radius: 5px;
                padding: 8px;
            }
        """)
        right_layout.addWidget(self.preview)
        
        # Stats
        self.stats = QLabel("Characters: 0 | Lines: 0")
        self.stats.setStyleSheet("color: #ffffff;")
        right_layout.addWidget(self.stats)
        
        layout.addWidget(right_panel, 1)
    
    def update_preview(self):
        """Update preview"""
        text = self.text_input.toPlainText()
        braille = translate_to_braille(text)
        self.preview.setText(braille)
        
        chars = len(text)
        lines = text.count('\n') + (1 if text.strip() else 0)
        self.stats.setText(f"Characters: {chars} | Lines: {lines}")
        
        if self.auto_read.isChecked() and text.strip():
            self.read_aloud()
    
    def read_aloud(self):
        """Read text aloud"""
        text = self.text_input.toPlainText().strip()
        if not text:
            self.status.setText("❌ No text")
            return
        
        self.status.setText("🔊 Reading...")
        self.tts_thread = TextToSpeechThread(text)
        self.tts_thread.finished.connect(lambda: self.status.setText("✅ Ready"))
        self.tts_thread.start()
    
    def print_to_arduino(self):
        """Send to Arduino"""
        text = self.text_input.toPlainText().strip()
        if not text:
            self.status.setText("❌ No text")
            return
        
        self.status.setText("🔌 Connecting to Arduino...")
        
        try:
            if not self.arduino.connect():
                self.status.setText("❌ Arduino not found")
                QMessageBox.critical(self, "Error", "Arduino not found!\n\nPlease connect Arduino via USB")
                return
            
            # Build command for Arduino
            lines = text.split('\n')
            self.status.setText(f"🖨️ Sending {len(lines)} line(s)...")
            
            for line_idx, line in enumerate(lines):
                line = line.strip()
                if not line:
                    continue
                
                # Simple command format
                cmd = f"PRINT:{line}\n"
                if self.arduino.send_command(cmd):
                    self.status.setText(f"✅ Line {line_idx + 1}/{len(lines)} sent")
                    time.sleep(0.5)
                else:
                    self.status.setText(f"❌ Error sending line {line_idx + 1}")
                    break
            
            self.arduino.disconnect()
            self.status.setText("✅ Complete!")
            QMessageBox.information(self, "Success", f"Sent {len(lines)} line(s) to Arduino!")
            
        except Exception as e:
            self.status.setText(f"❌ Error: {str(e)}")
            QMessageBox.critical(self, "Error", f"Error: {str(e)}")


def main():
    app = QApplication(sys.argv)
    window = BrailleDesktopApp()
    window.show()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()

