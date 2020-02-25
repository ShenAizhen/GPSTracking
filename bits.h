#ifndef BITS_H
#define BITS_H

class BitHelper{
public:
    static unsigned int bitExtracted(unsigned int number, int k, int p)
    {
        return (((1U << k) - 1) & (number >> p ));
    }

    static int single_bit_is_set(unsigned int value, int which){
        return (value >> which) & 1U;
    }

    //rev
    static int single_bit_is_setEx(unsigned char *value, int which){
        int index = which / 8;
        int num = which % 8;
        return (value[index] >> num) & 1U;
    }

    static unsigned int single_bit_set(unsigned int value, int which){
        value |= (1U << which);
        return value;
    }

    //rev
    static unsigned int single_bit_setEx(unsigned char* value, int which){
        int index = which / 8;
        int num = which % 8;
        value[index] |= (1U << num);
        return 0;
    }

    static unsigned int single_bit_clear(unsigned int value, int which){
        value &= ~(1U << which);
        return value;
    }

    //rev
    static unsigned int single_bit_clearEx(unsigned char* value, int which){
        int index = which / 8;
        int num = which % 8;
        value[index] &= ~(1U << num);
        return 0;
    }
};

#endif // BITS_H
