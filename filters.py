def up(x, b):
    return hex((x-b) & 0xff)

def sub(x, a):
    return hex((x-a) & 0xff)

def avg(x, a, b):
    return hex((x-((a+b)>>1) & 0xff) & 0xff)

def paeth(x, a, b, c):
    return hex((x-(paeth_pdr(a,b,c) & 0xff)) & 0xff)

# Use from filter imort * to use these functions in python env

def paeth_pdr(a,b,c):
    p = a+b-c

    pa = abs(p-a)
    pb = abs(p-b)
    pc = abs(p-c)

    if (pa <= pb) & (pa <= pc):
        return a
    elif pb <= pc:
        return b
    else:
        return c
    

def stb_paeth(a,b,c):
    thresh = c*3 - (a + b)
    if a<b: 
        lo = a
        hi = b 
    else: 
        lo = b
        hi = a
    
    if hi <= thresh:
        t0 = lo
    else:
        t0 = c

    if thresh <= lo:
        return hi
    else:
        return t0
    
def test(a,b,c):
    if a<c: 
        lo = a
        hi = c 
    else: 
        lo = c
        hi = a

    if b<lo: 
        return b
    elif b>hi:
        return hi


if __name__ == "__main__":
    # for a in range(256):
    #     for b in range(256):
    #         for c in range(256):
    #             if paeth_pdr(a,b,c) != test(a,b,c):
    #             # if paeth_pdr(a,b,c) != stb_paeth(a,b,c):
    #                 print(a, ",", b, ",", c)

    # print("None")

    print(hex(0xff - paeth_pdr(0x00,0x00,0xff)))