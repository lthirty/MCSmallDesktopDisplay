from PIL import Image
import io, sys

def png_to_jpeg_header(png_path, header_path, array_name, max_w, max_h, bg_color=(0,0,0)):
    img = Image.open(png_path).convert('RGBA')
    bg = Image.new('RGB', img.size, bg_color)
    bg.paste(img, mask=img.split()[3])
    img = bg
    ratio = min(max_w / img.width, max_h / img.height)
    new_w = max(1, int(img.width * ratio))
    new_h = max(1, int(img.height * ratio))
    img = img.resize((new_w, new_h), Image.LANCZOS)
    buf = io.BytesIO()
    img.save(buf, format='JPEG', quality=85)
    data = buf.getvalue()
    with open(header_path, 'w') as f:
        f.write('#include <pgmspace.h>\n')
        f.write('const uint8_t %s[] PROGMEM = {\n' % array_name)
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            line = ', '.join('0x%02X' % b for b in chunk)
            f.write('  %s,\n' % line)
        f.write('};\n')
    print('%s -> %s (%d bytes, %dx%d)' % (png_path, header_path, len(data), new_w, new_h))

png_to_jpeg_header('03.图片/横标白.png', 'img/boot_logo_lianli.h', 'boot_logo_lianli', 200, 70)
png_to_jpeg_header('03.图片/竖标白-纯图片.png', 'img/indoor_lianli.h', 'indoor_lianli', 60, 54)
