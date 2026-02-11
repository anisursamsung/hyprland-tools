#!/bin/bash

# get_content - Concatenate all files from a directory with section demarcation
# Usage: get_content <source_directory> <output_file>

# Function to display usage
show_usage() {
    echo "Usage: $0 <source_directory> <output_file>"
    echo "Both arguments must be absolute paths"
    echo ""
    echo "Example:"
    echo "  $0 /usr/include/hyprtoolkit /home/user/headers.txt"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -v, --verbose  Show verbose output"
    echo "  -e, --extensions <extensions>  File extensions to include (comma-separated)"
    echo "                                 Default: .hpp,.h,.cpp,.c,.cc,.cxx"
    exit 1
}

# Function to show error message and exit
show_error() {
    echo "Error: $1" >&2
    exit 1
}

# Default values
VERBOSE=false
EXTENSIONS=".hpp,.h,.cpp,.c,.cc,.cxx"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -e|--extensions)
            if [[ -n "$2" && "$2" != -* ]]; then
                EXTENSIONS="$2"
                shift 2
            else
                show_error "Extension list expected after -e/--extensions"
            fi
            ;;
        -*)
            show_error "Unknown option: $1"
            ;;
        *)
            # Positional arguments
            if [[ -z "$SOURCE_DIR" ]]; then
                SOURCE_DIR="$1"
            elif [[ -z "$OUTPUT_FILE" ]]; then
                OUTPUT_FILE="$1"
            else
                show_error "Too many arguments"
            fi
            shift
            ;;
    esac
done

# Check if we have both required arguments
if [[ -z "$SOURCE_DIR" || -z "$OUTPUT_FILE" ]]; then
    show_usage
fi

# Validate arguments
if [[ ! "$SOURCE_DIR" = /* ]]; then
    show_error "Source directory must be an absolute path: $SOURCE_DIR"
fi

if [[ ! "$OUTPUT_FILE" = /* ]]; then
    show_error "Output file must be an absolute path: $OUTPUT_FILE"
fi

if [[ ! -d "$SOURCE_DIR" ]]; then
    show_error "Source directory does not exist: $SOURCE_DIR"
fi

# Convert extensions to find pattern
IFS=',' read -ra EXT_ARRAY <<< "$EXTENSIONS"
FIND_PATTERN=""
for ext in "${EXT_ARRAY[@]}"; do
    ext="${ext#"${ext%%[![:space:]]*}"}"  # Trim leading whitespace
    ext="${ext%"${ext##*[![:space:]]}"}"  # Trim trailing whitespace
    FIND_PATTERN+=" -name \"*${ext}\" -o"
done
FIND_PATTERN="${FIND_PATTERN% -o}"  # Remove trailing " -o"

# Create output directory if it doesn't exist
OUTPUT_DIR=$(dirname "$OUTPUT_FILE")
if [[ ! -d "$OUTPUT_DIR" ]]; then
    if [[ "$VERBOSE" = true ]]; then
        echo "Creating output directory: $OUTPUT_DIR"
    fi
    mkdir -p "$OUTPUT_DIR" || show_error "Failed to create output directory: $OUTPUT_DIR"
fi

# Show verbose information
if [[ "$VERBOSE" = true ]]; then
    echo "Source directory: $SOURCE_DIR"
    echo "Output file:      $OUTPUT_FILE"
    echo "Extensions:       $EXTENSIONS"
    echo "Find pattern:     $FIND_PATTERN"
    echo ""
    echo "Starting file concatenation..."
fi

# Clear output file
> "$OUTPUT_FILE"

# Counters
TOTAL_FILES=0
PROCESSED_FILES=0
SKIPPED_FILES=0

# Find and process files
eval "find \"$SOURCE_DIR\" -type f \( $FIND_PATTERN \)" | sort | while read -r FILE; do
    TOTAL_FILES=$((TOTAL_FILES + 1))
    
    # Get relative path
    REL_PATH="${FILE#$SOURCE_DIR/}"
    REL_PATH="${REL_PATH#/}"  # Remove leading slash if present
    
    # Check if file is readable
    if [[ ! -r "$FILE" ]]; then
        if [[ "$VERBOSE" = true ]]; then
            echo "Skipping unreadable file: $REL_PATH"
        fi
        SKIPPED_FILES=$((SKIPPED_FILES + 1))
        continue
    fi
    
    if [[ "$VERBOSE" = true ]]; then
        echo "Processing: $REL_PATH"
    fi
    
    # Add section header with timestamp and file size
    FILE_SIZE=$(stat -c%s "$FILE" 2>/dev/null || echo "unknown")
    echo "=== $REL_PATH ===" >> "$OUTPUT_FILE"
    echo "Size: $FILE_SIZE bytes | Last modified: $(stat -c%y "$FILE" 2>/dev/null | cut -d. -f1)" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
    
    # Add file content
    if ! cat "$FILE" >> "$OUTPUT_FILE" 2>/dev/null; then
        echo "[ERROR: Failed to read file content]" >> "$OUTPUT_FILE"
        SKIPPED_FILES=$((SKIPPED_FILES + 1))
    else
        PROCESSED_FILES=$((PROCESSED_FILES + 1))
    fi
    
    # Add separator
    echo "" >> "$OUTPUT_FILE"
    echo "--- END OF: $REL_PATH ---" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
done

# Check if find command found any files
if [[ $TOTAL_FILES -eq 0 ]]; then
    echo "Warning: No files found with extensions: $EXTENSIONS" >&2
fi

# Print summary
echo "Done! Created $OUTPUT_FILE"
echo "Summary:"
echo "  Total files found:    $TOTAL_FILES"
echo "  Successfully processed: $PROCESSED_FILES"
echo "  Skipped files:        $SKIPPED_FILES"
echo "  Output lines:         $(wc -l < "$OUTPUT_FILE")"
echo "  Output size:          $(stat -c%s "$OUTPUT_FILE" 2>/dev/null || echo "unknown") bytes"