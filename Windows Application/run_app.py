#!/usr/bin/env python3
"""
SINC ROBOTICS Braille Embosser - Desktop Application Launcher
Double-click this file to run the complete application!
"""

import os
import sys
import time
import webbrowser
import subprocess
from pathlib import Path

# Get the directory where this script is located
APP_DIR = Path(__file__).parent.absolute()

def check_dependencies():
    """Check if required packages are installed"""
    required_packages = ['flask', 'pyserial']
    missing = []
    
    for package in required_packages:
        try:
            __import__(package)
        except ImportError:
            missing.append(package)
    
    if missing:
        print(f"❌ Missing packages: {', '.join(missing)}")
        print(f"Installing required packages...")
        for package in missing:
            os.system(f"pip install {package}")
        print("✅ Installation complete!")

def start_flask_server():
    """Start the Flask server"""
    os.chdir(APP_DIR)
    
    # Import Flask app
    from app import app, connect_arduino
    
    print("=" * 60)
    print("  SINC ROBOTICS - Braille Embosser Desktop Application")
    print("=" * 60)
    print(f"📁 Application Directory: {APP_DIR}")
    print("🔌 Connecting to Arduino...")
    
    connect_arduino()
    print("✅ Arduino connection initialized")
    
    print("🚀 Starting Flask server...")
    print("⏳ Server will be ready in a few seconds...")
    
    # Run Flask server
    app.run(
        host='127.0.0.1',
        port=5000,
        debug=False,
        use_reloader=False,
        use_debugger=False,
        threaded=True
    )

def open_browser():
    """Open the web interface in default browser"""
    time.sleep(2)  # Wait for Flask server to start
    
    url = 'http://127.0.0.1:5000'
    print(f"\n🌐 Opening browser: {url}")
    webbrowser.open(url)

if __name__ == '__main__':
    try:
        # Check dependencies
        check_dependencies()
        
        # Import threading for concurrent execution
        from threading import Thread
        
        # Start browser in background thread
        browser_thread = Thread(target=open_browser, daemon=True)
        browser_thread.start()
        
        # Start Flask server (blocking)
        print("\n" + "=" * 60)
        print("💡 Application is running!")
        print("📱 A browser window will open automatically...")
        print("🛑 To stop: Close this window or press Ctrl+C")
        print("=" * 60 + "\n")
        
        start_flask_server()
        
    except KeyboardInterrupt:
        print("\n\n👋 Application closed by user")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ Error: {e}")
        print("\n📝 Please ensure all files are in the same directory:")
        print("   - run_app.py")
        print("   - app.py")
        print("   - index.html")
        print("   - braille_parser.py (optional)")
        print("\n⏳ Press Enter to exit...")
        input()
        sys.exit(1)
