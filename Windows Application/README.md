# 🖨️ SINC ROBOTICS - Braille Embosser Desktop Application

## Complete Standalone Desktop Application
**Just double-click to run - everything works automatically!**

---

## 📋 Features

✅ **Desktop Application** - No command prompt needed
✅ **Auto-launch Browser** - Opens automatically on startup
✅ **Arduino Integration** - Automatic port detection and connection
✅ **Single & Multiple Line Printing** - Auto-detects input
✅ **Bangla & English Support** - Full language support
✅ **Poetry Mode** - Special formatting for poems
✅ **Real-time Preview** - See braille before printing
✅ **Text-to-Speech** - Read aloud functionality

---

## 🚀 Quick Start

### Option 1: Double-Click (Easiest)
1. Locate `SINC_ROBOTICS_Braille_Embosser.bat` in the application folder
2. **Double-click** to launch
3. Browser opens automatically with the application
4. Start printing!

### Option 2: Command Prompt
```bash
cd path\to\habijabi
python run_app.py
```
Then open browser to: `http://127.0.0.1:5000`

---

## ⚙️ Requirements

### Software
- **Python 3.7+** (Must be installed and added to PATH)
- **Windows, Mac, or Linux**

### Hardware
- **Arduino** (with CNC Shield & Stepper Motors)
- **USB Cable** (Arduino to Computer)

### Installation

#### 1. Install Python
- Download from: https://www.python.org/downloads/
- **IMPORTANT**: Check ✓ "Add Python to PATH" during installation
- Verify installation:
  ```bash
  python --version
  ```

#### 2. Install Required Packages
The application will auto-install these, or manually run:
```bash
pip install flask pyserial
```

---

## 📁 Project Structure

```
habijabi/
├── SINC_ROBOTICS_Braille_Embosser.bat  ← MAIN LAUNCHER (Double-click this!)
├── run_app.py                           ← Application launcher
├── app.py                               ← Flask backend & logic
├── index.html                           ← Web interface
├── braille_parser.py                    ← Braille translation
├── arduin.txt                           ← Arduino sketch
├── README.md                            ← This file
└── braille.bat                          ← Legacy launcher (optional)
```

---

## 🎯 How to Use

### 1. **Connect Arduino**
   - Plug Arduino into USB port
   - Application auto-detects on startup
   - LED will indicate connection

### 2. **Launch Application**
   - Double-click `SINC_ROBOTICS_Braille_Embosser.bat`
   - Browser opens with interface
   - System shows "System ready" message

### 3. **Enter Text**
   - Type in the text area (Bangla or English)
   - Preview updates in real-time
   - Check "This is a poem" for poetry mode

### 4. **Print**
   - Click "Print Braille" button
   - Single line → prints 1 line
   - Multiple lines → prints all lines sequentially
   - Status shows progress

### 5. **Additional Features**
   - **Preview Braille** - See braille representation
   - **Read Aloud** - Text-to-speech in Bengali/English
   - **Home Printer** - Reset to home position

---

## 🔧 Arduino Setup

### Upload Code to Arduino
1. Open Arduino IDE
2. Load `arduin.txt`
3. Select: Tools → Board → Arduino Uno
4. Select: Tools → Port → COM port with Arduino
5. Click Upload

### Circuit Connections
- **X-axis stepper** → X pins on CNC Shield
- **Y-axis stepper** → Y pins on CNC Shield
- **Solenoid** → Pin 9
- **Enable** → Pin 8

---

## ❌ Troubleshooting

### "Python not found"
- Install Python from https://www.python.org/
- **Restart computer after installation**
- Make sure "Add Python to PATH" was checked

### "Arduino offline"
- Check USB connection
- Upload code to Arduino using Arduino IDE
- Try different USB port
- Check Device Manager for COM port

### "Port 5000 in use"
- Close other applications using port 5000
- Or modify port in `app.py` line: `app.run(port=5001, ...)`

### "Browser not opening"
- Manually go to: http://127.0.0.1:5000
- Or http://localhost:5000

### "Nothing prints"
- Check Arduino connection (LED indicators)
- Verify stepper motor connections
- Check solenoid power supply
- Review browser console (F12) for errors

---

## 📱 Web Interface Guide

### Input Area (Left)
- **Text Field**: Enter text to print
- **Poetry Checkbox**: Enable for poem formatting
- **Preview**: Real-time braille display

### Buttons
- **Preview Braille**: Show braille representation
- **Read Aloud**: Text-to-speech
- **Print Braille**: Send to Arduino (auto-detects single/multiple lines)

### Status Messages
- ✅ Done - Successful print
- ❌ Offline - Arduino not connected
- ⚠️ Timeout - Communication timeout

---

## 🔄 Updates & Support

### File Organization
Keep all files in the same directory:
- `run_app.py`
- `app.py`
- `index.html`
- `SINC_ROBOTICS_Braille_Embosser.bat`

### Modifying Code
- Edit `app.py` for backend logic
- Edit `index.html` for UI/styling
- Edit `arduin.txt` for Arduino behavior
- Restart application for changes to take effect

---

## 📞 Support

**Issues?** Check:
1. All files in same directory
2. Python installed and in PATH
3. Arduino connected and code uploaded
4. Port 5000 not in use
5. Browser console for errors (F12)

---

## 🎓 Technical Stack

- **Frontend**: HTML5, Tailwind CSS, JavaScript
- **Backend**: Python Flask
- **Hardware Control**: PySerial + Arduino
- **Braille Logic**: Custom Bangla/English translation engine

---

## 📝 License

SINC ROBOTICS - Braille Embosser
All rights reserved.

---

**Version**: 1.0.0
**Last Updated**: December 2025
**Status**: ✅ Production Ready

**🎉 Enjoy printing in Braille!**
