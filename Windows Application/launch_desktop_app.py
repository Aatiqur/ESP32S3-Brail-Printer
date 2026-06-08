#!/usr/bin/env python3
"""
SINC ROBOTICS Desktop App Launcher
Checks dependencies and starts the desktop application
"""

import subprocess
import sys
import os

def check_and_install_dependencies():
    """Check if required packages are installed, install if missing"""
    packages = {
        'PyQt5': 'PyQt5==5.15.9',
        'pyttsx3': 'pyttsx3==2.90',
        'pyserial': 'pyserial==3.5'
    }
    
    print("=" * 50)
    print("SINC ROBOTICS Braille Embosser Desktop")
    print("=" * 50)
    print()
    
    missing = []
    for package_name, package_spec in packages.items():
        try:
            __import__(package_name)
            print(f"✅ {package_name} is installed")
        except ImportError:
            print(f"❌ {package_name} is missing")
            missing.append(package_spec)
    
    if missing:
        print()
        print("Installing missing packages...")
        for package in missing:
            print(f"  Installing {package}...")
            subprocess.check_call([sys.executable, '-m', 'pip', 'install', package, '-q'])
        print("✅ All packages installed!")
    
    print()
    return True

def main():
    """Main entry point"""
    try:
        # Check dependencies
        if not check_and_install_dependencies():
            return 1
        
        # Import and run the app
        print("Starting application...")
        from braille_desktop_app import main as app_main
        app_main()
        
    except Exception as e:
        print(f"❌ Error: {e}")
        input("Press Enter to exit...")
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
