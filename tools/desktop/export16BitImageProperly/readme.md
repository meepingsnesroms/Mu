# Reexports GIMP C source images properly

Because GIMP exports 16 bit images as little endian in a byte array of octal chars using non fixed size data types or as macro accessed byte array.  
This program takes the macro accessed byte array.  
That breaks big endian platforms and universal compatibility.
