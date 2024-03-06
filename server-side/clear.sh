colored_echo() {
    local color_code="$1"
    local text="$2"
    case "$OS" in
        Darwin)
            # macOS uses \033 instead of \e for color codes
            echo -e "\033[${color_code}${text}\033[0m"
            ;;
        *)
            # Linux and other systems
            echo -e "\e[${color_code}${text}\e[0m"
            ;;
    esac
}

colored_echo "32m--------|" "Delete DB |--------"
rm music.db
echo "    DONE"
colored_echo "32m--------|" "Delete all mp3 files |--------"
rm music/*.mp3
echo "    DONE"