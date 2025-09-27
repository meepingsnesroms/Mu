
# Universal Makefile for Mu

# Default target: Display help
.PHONY: all
all: help

# Target to build the Qt application
.PHONY: qt
qt:
	@echo "Building Qt application..."
	@(cd qtBuildSystem/Mu && qmake6 && $(MAKE))

# Target to build the libretro core
# Usage: make libretro [PLATFORM=platform_name]
# Example: make libretro PLATFORM=linux_x86_64
.PHONY: libretro
libretro:
	@echo "Building libretro core..."
	@(cd libretroBuildSystem && $(MAKE) -f Makefile.libretro platform=$(PLATFORM))

# Target to clean up build artifacts
.PHONY: clean
clean:
	@echo "Cleaning up..."
	@(cd qtBuildSystem/Mu && $(MAKE) clean)
	@(cd libretroBuildSystem && $(MAKE) -f Makefile.libretro clean)

install:
	@echo "Installing Mu..."
	@sudo cp qtBuildSystem/Mu/Mu /usr/local/bin/
	@sudo cp images/Mu.svg /usr/share/icons/hicolor/scalable/apps/
	@echo "[Desktop Entry]\nName=Mu\nComment=A newer Palm OS emulator\nExec=/usr/local/bin/Mu\nIcon=Mu\nTerminal=false\nType=Application\nCategories=Game;Emulator;" | sudo tee /usr/share/applications/Mu.desktop > /dev/null
	@sudo gtk-update-icon-cache /usr/share/icons/hicolor/

uninstall:
	@echo "Uninstalling Mu..."
	@sudo rm -f /usr/local/bin/Mu
	@sudo rm -f /usr/share/icons/hicolor/scalable/apps/Mu.svg
	@sudo rm -f /usr/share/applications/Mu.desktop
	@sudo gtk-update-icon-cache /usr/share/icons/hicolor/

# Target to display help
.PHONY: help
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  qt         - Build the Qt application"
	@echo "  libretro   - Build the libretro core"
	@echo "  clean      - Clean up build artifacts"
	@echo ""
	@echo "For libretro, you can specify a platform:"
	@echo "  make libretro PLATFORM=linux_x86_64"
	@echo ""

