#!/bin/bash
# make.sh - Advanced Kernel & Userspace Build Orchestration

usage() {
    echo "Usage: $0 <action> <targets>"
    echo "Actions:"
    echo "  kb - Kernel Build          kc - Kernel Clean"
    echo "  ub - User Build            uc - User Clean"
    echo "  bb - Both Build (K + U)    bc - Both Clean (K + U)"
    echo "  pf - Print Files (Debug)"
    echo "Targets:"
    echo "  1. subfolder"
    echo "  2. subfolder1 subfolder2 ..."
    echo "  3. all"
    exit 1
}

# Ensure at least an action and one target are provided
[ $# -lt 2 ] && usage

ACTION=$1
shift   # Remove the action string, leaving only targets ($@)

# Define the central output folder path
OUTPUT_DIR="/home/siraj/Code/ST32MP257P-DK/KERNELSPACE/___output_files"

WINDIR="/mnt/s/_______UBUNTU________/stm32_modules_application"

# Combine remaining arguments, split by commas or spaces into an array
TARGETS=$(echo "$@" | tr ',' ' ')
FOLDERS=($TARGETS)

# Expand "all" into all subfolders containing a Makefile
if [ "${FOLDERS[0]}" = "all" ]; then
    FOLDERS=()
    for d in */; do
        [ -f "$d/Makefile" ] && FOLDERS+=("${d%/}")
    done
fi

# Helper function to build Kernel Module
build_kernel() {
    local folder=$1
    if [ -f "$folder/Makefile" ]; then
        echo ">>> [Kernel] Building in $folder"
        make -C "$folder"
    else
        echo "!!! Skipping Kernel Build in $folder (no Makefile)"
    fi
}

# Helper function to clean Kernel Module
clean_kernel() {
    local folder=$1
    if [ -f "$folder/Makefile" ]; then
        echo ">>> [Kernel] Cleaning in $folder"
        make -C "$folder" clean
    else
        echo "!!! Skipping Kernel Clean in $folder (no Makefile)"
    fi
}

# Helper function to find and build Userspace Binary
build_userspace() {
    local folder=$1
    local user_src=""

    for file in "$folder"/*.c; do
        [ -e "$file" ] || continue
        if grep -qE "int[[:space:]]+main" "$file"; then
            user_src="$file"
            break
        fi
    done

    if [ -z "$user_src" ]; then
        echo ">>> [Userspace] No file with main() found in $folder"
        return
    fi

    local exe_name=$(basename "$user_src" .c)

    if [ "$folder" = "25c.Netlink_Socket_MultiCast" ]; then
        echo ">>> [Userspace] Found $user_src -> Compiling with multicast libs to $folder/$exe_name"
        gcc "$user_src" -o "$folder/$exe_name" $(pkg-config --cflags --libs libnl-3.0 libnl-genl-3.0)

        if [ $? -eq 0 ]; then
            echo ">>> [Userspace] Build successful!"

            echo ">>> [Userspace] Copying executable to $OUTPUT_DIR"
            mkdir -p "$OUTPUT_DIR"
            cp "$folder/$exe_name" "$OUTPUT_DIR/"

            echo ">>> [Userspace] Copying executable to $WINDIR"
            cp "$folder/$exe_name" "$WINDIR/"
        else
            echo "!!! [Userspace] Compilation failed for $user_src"
        fi
        return
    fi

    echo ">>> [Userspace] Found $user_src -> Compiling to $folder/$exe_name"
    aarch64-linux-gnu-gcc "$user_src" -o "$folder/$exe_name"

    if [ $? -eq 0 ]; then
        echo ">>> [Userspace] Build successful!"

        # Copy to the output directory ---
        echo ">>> [Userspace] Copying executable to $OUTPUT_DIR"
        mkdir -p "$OUTPUT_DIR"
        cp "$folder/$exe_name" "$OUTPUT_DIR/"

        # Also copy to the Windows directory ---
        echo ">>> [Userspace] Copying executable to $WINDIR"
        cp "$folder/$exe_name" "$WINDIR/"

    else
        echo "!!! [Userspace] Compilation failed for $user_src"
    fi
}

# Helper function to find and clean Userspace Binary
clean_userspace() {
    local folder=$1

    for file in "$folder"/*.c; do
        [ -e "$file" ] || continue
        if grep -qE "int[[:space:]]+main" "$file"; then
            local exe_name=$(basename "$file" .c)
            
            # Clean local subfolder binary
            if [ -f "$folder/$exe_name" ]; then
                echo ">>> [Userspace] Removing binary $folder/$exe_name"
                rm -f "$folder/$exe_name"
            fi
            
            # Also clean from the output directory ---
            if [ -f "$OUTPUT_DIR/$exe_name" ]; then
                echo ">>> [Userspace] Removing binary from $OUTPUT_DIR/$exe_name"
                rm -f "$OUTPUT_DIR/$exe_name"
            fi
            break
        fi
    done
}

# Helper function to trigger the Makefile debug print
print_makefile_files() {
    local folder=$1
    if [ -f "$folder/Makefile" ]; then
        echo ">>> [Makefile Debug] Inspecting files in $folder"
        make -C "$folder" print-files
    else
        echo "!!! Skipping Print in $folder (no Makefile)"
    fi
}

# Main Execution Routing
case $ACTION in
    kb)
        echo "========================================="
        echo ">>> ACTION: Kernel Build"
        echo "========================================="
        for d in "${FOLDERS[@]}"; do
            build_kernel "$d"
            echo "-----------------------------------------"
        done
        ;;
    kc)
        echo "========================================="
        echo ">>> ACTION: Kernel Clean"
        echo "========================================="
        for d in "${FOLDERS[@]}"; do
            clean_kernel "$d"
            echo "-----------------------------------------"
        done
        ;;
    ub)
        echo "========================================="
        echo ">>> ACTION: Userspace Build"
        echo "========================================="
        for d in "${FOLDERS[@]}"; do
            build_userspace "$d"
            echo "-----------------------------------------"
        done
        ;;
    uc)
        echo "========================================="
        echo ">>> ACTION: Userspace Clean"
        echo "========================================="
        for d in "${FOLDERS[@]}"; do
            clean_userspace "$d"
            echo "-----------------------------------------"
        done
        ;;
    bb)
        echo "========================================="
        echo ">>> ACTION: Both Build (Kernel + Userspace)"
        echo "========================================="
        for d in "${FOLDERS[@]}"; do
            build_kernel "$d"
            build_userspace "$d"
            echo "-----------------------------------------"
        done
        ;;
    bc)
        echo "========================================="
        echo ">>> ACTION: Both Clean (Kernel + Userspace)"
        echo "========================================="
        for d in "${FOLDERS[@]}"; do
            clean_kernel "$d"
            clean_userspace "$d"
            echo "-----------------------------------------"
        done
        ;;
    pf)
        echo "========================================="
        echo ">>> ACTION: Printing Evaluated Files from Makefile"
        echo "========================================="
        for d in "${FOLDERS[@]}"; do
            print_makefile_files "$d"
            echo "-----------------------------------------"
        done
        ;;
    *)
        usage
        ;;
esac