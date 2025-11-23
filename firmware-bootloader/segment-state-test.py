import serial # pyright: ignore[reportMissingModuleSource]

serial = serial.Serial(port='/dev/tty.usbmodem11303', baudrate=115200)










while(True):
    segment_length = serial.read(1)
    segment_type = serial.read(1)
    data = []
    for i in range(32):
        data.append(serial.read(1))
    segment_crc = serial.read(1)
    
    segment_length_converted = segment_length[0]
    segment_type_converted = segment_type[0]
    segment_crc_converted = segment_crc[0]

    print(f"Segment Length = {segment_length_converted}")
    print(f"Segment Type = {segment_type_converted}")
    for i in range(32):
        print(f"Data[{i}] = {data[i]}")
    print(f"Segment CRC = {segment_crc_converted}")
    print("\n")
