#!/usr/bin/env python3
import os
import sys

def png_to_c_header(png_file):
    """Convert PNG file to C header with hex array"""
    with open(png_file, 'rb') as f:
        data = f.read()
    
    # Get base name without extension
    base_name = os.path.splitext(os.path.basename(png_file))[0]
    var_name = base_name.replace('-', '_')
    
    # Rename to avoid conflicts and reserved keywords
    if var_name == 'controller_base':
        var_name = 'keypad_frame_graphic'
    elif var_name == 'default':
        var_name = 'default_keypad_image'
    
    # Create header file
    header_file = png_file.replace('.png', '.h')
    
    # Generate C code
    hex_bytes = ', '.join(f'0x{b:02x}' for b in data)
    
    c_code = f"""unsigned char {var_name}[] = {{
  {hex_bytes}
}};
unsigned int {var_name}_len = {len(data)};
"""
    
    # Format nicely - break lines every ~80 chars
    lines = c_code.split('\n')
    formatted_lines = []
    for line in lines:
        if line.startswith('  0x'):
            # Break long hex lines
            formatted_line = '  '
            for i, hex_pair in enumerate(hex_bytes.split(', ')):
                formatted_line += hex_pair + ', '
                if (i + 1) % 16 == 0:
                    formatted_lines.append(formatted_line.rstrip(', '))
                    formatted_line = '  '
            if formatted_line.strip():
                formatted_lines.append(formatted_line.rstrip(', '))
        else:
            formatted_lines.append(line)
    
    # Simpler approach - just write the hex with line breaks every 16 bytes
    hex_lines = []
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_chunk = ', '.join(f'0x{b:02x}' for b in chunk)
        hex_lines.append('  ' + hex_chunk)
    
    hex_joined = ',\n'.join(hex_lines)
    c_code = f"""unsigned char {var_name}[] = {{
{hex_joined}
}};
unsigned int {var_name}_len = {len(data)};
"""
    
    with open(header_file, 'w') as f:
        f.write(c_code)
    
    print(f"Created {header_file} ({len(data)} bytes)")

if __name__ == '__main__':
    os.chdir(r's:\VisualStudio\FreeIntvTSOverlay\Assets')
    for png_file in ['banner.png', 'keypad_frame_graphic.png', 'default_keypad_image.png']:
        png_to_c_header(png_file)
