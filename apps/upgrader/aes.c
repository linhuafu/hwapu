#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include <sys/time.h>
struct timeval tv_start, tv_over;

// The number of columns comprising a state in AES. This is a constant in AES. Value=4
#define Nb 4

// The number of rounds in AES Cipher. It is simply initiated to zero. The actual value is recieved in the program.
static int Nr=10; //   Nr = Nk + 6; //Nr=4+6=10

// The number of 32 bit words in the key. It is simply initiated to zero. The actual value is recieved in the program.
static int Nk=4;//   Nk = Nc / 32;//Nc=128针对128位密钥做针对处理
 
//Nc: the length of Key(128, 192 or 256) only
static int Nc = 128;
//int Nc = 256;
//inline void InvMixColumns()函数中用到

// in - it is the array that holds the CipherText to be decrypted.
// out - it is the array that holds the output of the for decryption.
// state - the array that holds the intermediate results during decryption.
//unsigned char in[16], out[16], state[4][4];
static unsigned char  state[4][4];
// The array that stores the round keys.
static unsigned char RoundKey[240];

// The Key input to the AES Program
unsigned char Key[16]="0123456789012345";
//unsigned char Key[32];

//int getSBoxInvert(int num)
static int rsbox[256] =
{ 0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb
, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb
, 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e
, 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25
, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92
, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84
, 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06
, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b
, 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73
, 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e
, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b
, 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4
, 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f
, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef
, 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61
, 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d };


//int getSBoxValue(int num)
static int sbox[256] =   {
    //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };


// The round constant word array, Rcon[i], contains the values given by 
// x to th e power (i-1) being powers of x (x is denoted as {02}) in the field GF(2^8)
// Note that i starts at 1, not 0).
static int Rcon[255] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 
    0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 
    0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 
    0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 
    0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 
    0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 
    0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 
    0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 
    0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 
    0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 
    0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 
    0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 
    0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 
    0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 
    0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 
    0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb  };

// This function produces Nb(Nr+1) round keys. The round keys are used in each round to decrypt the states. 
void KeyExpansion()
{
    int i,j;
    unsigned char temp[4],k;
    
    // The first round key is the key itself.
    /*for(i=0;i<Nk;i++)//Nk=4;
    {
        RoundKey[i*4]=Key[i*4];
        RoundKey[i*4+1]=Key[i*4+1];
        RoundKey[i*4+2]=Key[i*4+2];
        RoundKey[i*4+3]=Key[i*4+3];
    }*/ //for()等价于memcpy(RoundKey,Key,16); i=4;
    memcpy(RoundKey,Key,16);
		i=4;
    // All other round keys are found from the previous round keys.
    while (i < (Nb * (Nr+1)))
    {
        for(j=0;j<4;j++)
        {
            temp[j]=RoundKey[(i-1) * 4 + j];
        }
        if (i % Nk == 0)
        {
            // This function rotates the 4 bytes in a word to the left once.
            // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]
            {
                k = temp[0];
                temp[0] = temp[1];
                temp[1] = temp[2];
                temp[2] = temp[3];
                temp[3] = k;
            }

            // SubWord() is a function that takes a four-byte input word and 
            // applies the S-box to each of the four bytes to produce an output word.
            {
                temp[0]=sbox[temp[0]];
                temp[1]=sbox[temp[1]];
                temp[2]=sbox[temp[2]];
                temp[3]=sbox[temp[3]];
            }

            temp[0] =  temp[0] ^ Rcon[i/Nk];
        }
        else if (Nk > 6 && i % Nk == 4)
        {
            {
                temp[0]=sbox[temp[0]];
                temp[1]=sbox[temp[1]];
                temp[2]=sbox[temp[2]];
                temp[3]=sbox[temp[3]];
            }
        }
        RoundKey[i*4+0] = RoundKey[(i-Nk)*4+0] ^ temp[0];
        RoundKey[i*4+1] = RoundKey[(i-Nk)*4+1] ^ temp[1];
        RoundKey[i*4+2] = RoundKey[(i-Nk)*4+2] ^ temp[2];
        RoundKey[i*4+3] = RoundKey[(i-Nk)*4+3] ^ temp[3];
        i++;
    }
}

// This function adds the round key to state.
// The round key is added to the state by an XOR function.
inline void AddRoundKey(int round) 
{
    int i,j;
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
           //state[j][i] ^= RoundKey[round * Nb * 4 + i * Nb + j];
           //Nb=4
           state[j][i] ^= RoundKey[round *16 + i * 4 + j];
        }
    }
}

// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
inline void InvSubBytes()
{
    int i,j;
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
          state[i][j] = rsbox[state[i][j]];
        }
    }
}

// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
inline void InvShiftRows()
{
    unsigned char temp;

    // Rotate first row 1 columns to right    
    temp=state[1][3];
    state[1][3]=state[1][2];
    state[1][2]=state[1][1];
    state[1][1]=state[1][0];
    state[1][0]=temp;

    // Rotate second row 2 columns to right    
    temp=state[2][0];
    state[2][0]=state[2][2];
    state[2][2]=temp;

    temp=state[2][1];
    state[2][1]=state[2][3];
    state[2][3]=temp;

    // Rotate third row 3 columns to right
    temp=state[3][0];
    state[3][0]=state[3][1];
    state[3][1]=state[3][2];
    state[3][2]=state[3][3];
    state[3][3]=temp;
}

// xtime is a macro that finds the product of {02} and the argument to xtime modulo {1b}  
#define xtime(x)   ((x<<1) ^ (((x>>7) & 1) * 0x1b))

// Multiplty is a macro used to multiply numbers in the field GF(2^8)
//#define Multiply(x,y) (((y & 1) * x) ^ ((y>>1 & 1) * xtime(x)) ^ ((y>>2 & 1) * xtime(xtime(x))) ^ ((y>>3 & 1) * xtime(xtime(xtime(x)))) ^ ((y>>4 & 1) * xtime(xtime(xtime(xtime(x))))))
/*
#define Multiply_0x0e(x) ((0) ^ (xtime(x)) ^ ( xtime(xtime(x))) ^ ( xtime(xtime(xtime(x)))) ^ (0))  //0x0e=1110
#define Multiply_0x09(x) ((x) ^ (0) ^ (0) ^ (xtime(xtime(xtime(x)))) ^ (0))    //0x09=1001
#define Multiply_0x0d(x) ((x) ^ (0) ^ (xtime(xtime(x))) ^ (xtime(xtime(xtime(x)))) ^ (0))  //0x0d=1101
#define Multiply_0x0b(x) ((x) ^ (xtime(x)) ^ (0) ^  (xtime(xtime(xtime(x)))) ^ (0))//0x0b=1011

#define Multiply_0x0e(x) ((xtime(x)) ^ ( xtime(xtime(x))) ^ ( xtime(xtime(xtime(x)))))  //0x0e=1110
#define Multiply_0x09(x) ((0) ^ (x) ^ (xtime(xtime(xtime(x)))))    //0x09=1001
#define Multiply_0x0d(x) ((x)  ^ (xtime(xtime(x))) ^ (xtime(xtime(xtime(x)))) )  //0x0d=1101
#define Multiply_0x0b(x) ((x) ^ (xtime(x)) ^  (xtime(xtime(xtime(x)))) )//0x0b=1011
*/

// MixColumns function mixes the columns of the state matrix.
// The method used to multiply may be difficult to understand for the inexperienced.
// Please use the references to gain more information.
inline void InvMixColumns()
{
 int i,j;
 unsigned char a,b,c,d;
 unsigned char a1,a2,a3;
 unsigned char b1,b2,b3;
 unsigned char c1,c2,c3;
 unsigned char d1,d2,d3;

 unsigned char a0_3,b0_3,c0_3,d0_3;

 unsigned char a_10;
 unsigned char M_a_10_b0_3;
 unsigned char M_a0_3_d0_3;
 unsigned char c_10;
 unsigned char M_b2_d2;
 unsigned char M_a2_c2;
 unsigned char M_M_b2_d2_c0_3;
   
    for(i=0;i<4;i++)
    {    
        a = state[0][i];
        b = state[1][i];
        c = state[2][i];
        d = state[3][i];
    		//add by tgh
        a1=xtime(a);
        a2=xtime(a1);
        a3=xtime(a2);
        a0_3=a^a3;
        
        b1=xtime(b);
        b2=xtime(b1);
        b3=xtime(b2);
        b0_3=b^b3;
        
        c1=xtime(c);
        c2=xtime(c1);
        c3=xtime(c2);
        c0_3=c^c3;
        
        d1=xtime(d);
        d2=xtime(d1);
        d3=xtime(d2);
       	d0_3=d^d3;
       	
       	
       	a_10=a1^0;
       	M_a_10_b0_3=a_10^b0_3;
       	M_a0_3_d0_3=a0_3^d0_3;
       	c_10=c1^0;
       	M_b2_d2=b2^d2;
       	M_a2_c2=a2^c2;
       	M_M_b2_d2_c0_3=M_b2_d2^c0_3;
       	
        state[0][i] = M_a_10_b0_3 ^ M_a2_c2 ^ a3 ^ b1 ^ c0_3  ^ d0_3;
        state[1][i] = M_a0_3_d0_3 ^  b1 ^ M_M_b2_d2_c0_3 ^ b3 ^ c_10 ;
        state[2][i] = M_a0_3_d0_3 ^ M_a2_c2  ^ b0_3 ^ c_10 ^ c3  ^ d1;
        state[3][i] = M_a_10_b0_3 ^ a0_3 ^ M_M_b2_d2_c0_3 ^ d1 ^ d3;      	
 	
		/*  	      	
       	state[0][i] = (a1 ^ a2 ^ a3) ^ ( b1 ^ b0_3 ) ^ (c2 ^ c0_3) ^ (0 ^ d0_3);
        state[1][i] = (0 ^ a0_3) ^ (b1 ^ b2 ^ b3) ^ (c1 ^ c0_3 ) ^ (d2 ^ d0_3);
        state[2][i] = (a2 ^ a0_3) ^ (0 ^ b0_3) ^ (c1 ^ c2 ^ c3 ) ^ (d1 ^ d0_3);
        state[3][i] = (a1 ^ a0_3) ^ (b2 ^ b0_3) ^ (0 ^ c0_3 ) ^ (d1 ^ d2 ^ d3); 
     */
     /*   
        state[0][i] = (a1 ^ a2 ^ a3) ^ (b ^ b1 ^ b3 ) ^ (c ^ c2 ^ c3) ^ (0 ^ d ^ d3);
        state[1][i] = (0 ^ a ^ a3) ^ (b1 ^ b2 ^ b3) ^ (c ^ c1 ^ c3 ) ^ (d  ^ d2 ^ d3);
        state[2][i] = (a ^ a2 ^ a3) ^ (0 ^ b ^ b3) ^ (c1 ^ c2 ^ c3 ) ^ (d  ^ d1 ^ d3);
        state[3][i] = (a ^ a1 ^ a3) ^ (b ^ b2 ^ b3) ^ (0 ^ c ^ c3 ) ^ (d1 ^ d2 ^ d3); 
     */    
     /* state[0][i] = Multiply_0x0e(a) ^ Multiply_0x0b(b) ^ Multiply_0x0d(c) ^ Multiply_0x09(d);
        state[1][i] = Multiply_0x09(a) ^ Multiply_0x0e(b) ^ Multiply_0x0b(c) ^ Multiply_0x0d(d);
        state[2][i] = Multiply_0x0d(a) ^ Multiply_0x09(b) ^ Multiply_0x0e(c) ^ Multiply_0x0b(d);
        state[3][i] = Multiply_0x0b(a) ^ Multiply_0x0d(b) ^ Multiply_0x09(c) ^ Multiply_0x0e(d); 
         
        state[0][i] = Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09);
        state[1][i] = Multiply(a, 0x09) ^ Multiply(b, 0x0e) ^ Multiply(c, 0x0b) ^ Multiply(d, 0x0d);
        state[2][i] = Multiply(a, 0x0d) ^ Multiply(b, 0x09) ^ Multiply(c, 0x0e) ^ Multiply(d, 0x0b);
        state[3][i] = Multiply(a, 0x0b) ^ Multiply(b, 0x0d) ^ Multiply(c, 0x09) ^ Multiply(d, 0x0e);
    */
    }
}

// InvCipher is the main function that decrypts the CipherText.

//合并decrypt()和InvCipher()两个函数为Decrypt_InvCipher() 
void Decrypt_InvCipher(char *str)
{
    int round=0;
		int i,j;
    //Copy the input CipherText to state array.
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            state[j][i] = str[i*4 + j];
        }
    }

    // Add the First round key to the state before starting the rounds.
    AddRoundKey(Nr); 

    // There will be Nr rounds.
    // The first Nr-1 rounds are identical.
    // These Nr-1 rounds are executed in the loop below.
    for(round=Nr-1;round>0;round--)
    {
    	  
        InvShiftRows();
        InvSubBytes();
        AddRoundKey(round);
        InvMixColumns();  
      /*
        gettimeofday(&tv_start, NULL);
        InvShiftRows();
        gettimeofday(&tv_over, NULL);
        printf("InvShiftRows() cost time:%d us\n",(tv_over.tv_sec-tv_start.tv_sec)*1000000+(tv_over.tv_usec-tv_start.tv_usec));
        
        gettimeofday(&tv_start, NULL);
        InvSubBytes();
        gettimeofday(&tv_over, NULL);
        printf("InvSubBytes() cost time:%d us\n",(tv_over.tv_sec-tv_start.tv_sec)*1000000+(tv_over.tv_usec-tv_start.tv_usec));
        
        
        gettimeofday(&tv_start, NULL);
        AddRoundKey(round);
        gettimeofday(&tv_over, NULL);
        printf("AddRoundKey() cost time:%d us\n",(tv_over.tv_sec-tv_start.tv_sec)*1000000+(tv_over.tv_usec-tv_start.tv_usec));
        
        gettimeofday(&tv_start, NULL);
        InvMixColumns();
        gettimeofday(&tv_over, NULL);
        printf("InvMixColumns()new cost time:%d us\n",(tv_over.tv_sec-tv_start.tv_sec)*1000000+(tv_over.tv_usec-tv_start.tv_usec));
       */ 
    }
    // The last round is given below.
    // The MixColumns function is not here in the last round.
    InvShiftRows();
    InvSubBytes();
    AddRoundKey(0);

    // The decryption process is over.
    // Copy the state array to output array.
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            str[i*4+j]=state[j][i];
        }
    }
}


/*------------------------------
----------------    加密的部分,我们不用



// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
void SubBytes()
{
    int i,j;
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            state[i][j] = sbox[state[i][j]];
        }
    }
}

// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
void ShiftRows()
{
    unsigned char temp;

    // Rotate first row 1 columns to left    
    temp=state[1][0];
    state[1][0]=state[1][1];
    state[1][1]=state[1][2];
    state[1][2]=state[1][3];
    state[1][3]=temp;

    // Rotate second row 2 columns to left    
    temp=state[2][0];
    state[2][0]=state[2][2];
    state[2][2]=temp;

    temp=state[2][1];
    state[2][1]=state[2][3];
    state[2][3]=temp;

    // Rotate third row 3 columns to left
    temp=state[3][0];
    state[3][0]=state[3][3];
    state[3][3]=state[3][2];
    state[3][2]=state[3][1];
    state[3][1]=temp;
}

// xtime is a macro that finds the product of {02} and the argument to xtime modulo {1b}  
#define xtime(x)   ((x<<1) ^ (((x>>7) & 1) * 0x1b))

// MixColumns function mixes the columns of the state matrix
void MixColumns()
{
    int i;
    unsigned char Tmp,Tm,t;
    for(i=0;i<4;i++)
    {    
        t=state[0][i];
        Tmp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i] ;
        Tm = state[0][i] ^ state[1][i] ; Tm = xtime(Tm); state[0][i] ^= Tm ^ Tmp ;
        Tm = state[1][i] ^ state[2][i] ; Tm = xtime(Tm); state[1][i] ^= Tm ^ Tmp ;
        Tm = state[2][i] ^ state[3][i] ; Tm = xtime(Tm); state[2][i] ^= Tm ^ Tmp ;
        Tm = state[3][i] ^ t ; Tm = xtime(Tm); state[3][i] ^= Tm ^ Tmp ;
    }
}

// Cipher is the main function that encrypts the PlainText.

void Cipher()
{
    int i,j,round=0;

    //Copy the input PlainText to state array.
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            state[j][i] = in[i*4 + j];
        }
    }

    // Add the First round key to the state before starting the rounds.
    AddRoundKey(0); 
    
    // There will be Nr rounds.
    // The first Nr-1 rounds are identical.
    // These Nr-1 rounds are executed in the loop below.
    for(round=1;round<Nr;round++)
    {
        SubBytes();
        ShiftRows();
        MixColumns();
        AddRoundKey(round);
    }
    
    // The last round is given below.
    // The MixColumns function is not here in the last round.
    SubBytes();
    ShiftRows();
    AddRoundKey(Nr);

    // The encryption process is over.
    // Copy the state array to output array.
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            out[i*4+j]=state[j][i];
        }
    }
}

int  encrypt(char *str, char *key, char *encrypt_out) 
{
    int i,j,Nl;
	double len;
	char *newstr;
	char *str2;

    Nk = Nc / 32;//Nc=128
    Nr = Nk + 6; //Nr=4+6=10
    
    //len= strlen(str);
    len= 16;
    Nl = (int)ceil(len / 16);
    
    printf("encrypt --->Nk:%d,Nr:%d,Nl:%d\n", Nk,Nr,Nl);        
    
    for(i=0;i<Nl;i++)
    {
        for(j=0;j<Nk*4;j++)
        {
            Key[j]=key[j];
            in[j]=str[i*16+j];
        }
        
        KeyExpansion();
        Cipher();
        //strcat(newstr,out);
        //memcpy(encrypt_out,out,sizeof(out));
        memcpy(encrypt_out,out, 16);
    }
    return 0;
}
*/
/*int decrypt(char *str) 
{
   // int i,j,len,Nl;

    //printf("decrypt --->Nk:%d,Nr:%d,Nl:%d\n", Nk,Nr,Nl);        

   memcpy(in,str,16);
        
    printf("keyExpansion start\n");
   gettimeofday(&tv_start, NULL);
   KeyExpansion();
   gettimeofday(&tv_over, NULL);
   printf("KeyExpansion() cost time:%d us\n",(tv_over.tv_sec-tv_start.tv_sec)*1000000+(tv_over.tv_usec-tv_start.tv_usec));
   // printf("keyExpansion stop\n");
   // printf("InvCipheer start\n");
   //gettimeofday(&tv_start, NULL);
   InvCipher();
   //gettimeofday(&tv_over, NULL);
   //printf("InvCipher() cost time:%d us\n",(tv_over.tv_sec-tv_start.tv_sec)*1000000+(tv_over.tv_usec-tv_start.tv_usec));
   //  printf("InvCipheer stop\n");
   //strcat(newstr,out);
   //memcpy(decrypt_out,out,sizeof(out));
   /* memcpy(decrypt_out,out, 16);
    return 0;
} 
*/
/*int decrypt(char *str, char *key, char *decrypt_out) 
{
    int i,j,len,Nl;
	char *newstr;

    Nk = Nc / 32;//Nc=128
    Nr = Nk + 6; //Nr=4+6=10
    
    //len= strlen(str);
    len= 16;
    Nl = (int)ceil(len / 16);
     
    printf("decrypt --->Nk:%d,Nr:%d,Nl:%d\n", Nk,Nr,Nl);        

    for(i=0;i<Nl;i++)
    {
        for(j=0;j<Nk*4;j++)
        {
            Key[j]=key[j];
            in[j]=str[i*16+j];
        }
        
        KeyExpansion();
        InvCipher();
        //strcat(newstr,out);
        //memcpy(decrypt_out,out,sizeof(out));
        memcpy(decrypt_out,out, 16);
    }
    return 0;
}*/

/*int main()
{
	unsigned char encrypt_out[16];
	unsigned char decrypt_out[16];
	unsigned char aes_key[32];
	unsigned char inbuf[16] ;
	int i;
	char *key = "01234567890123456789012345678901";
	//char *buf = "1234567890qwerty";
	char *buf = "u";

	memset(aes_key,0 ,sizeof(aes_key));
	memcpy(aes_key, key, sizeof(aes_key));
	memset(inbuf,0 ,sizeof(inbuf));
	memcpy(inbuf, buf, strlen(buf));
	printf("inbuf result:%s, len:%d\n", inbuf,sizeof(inbuf));
 	for(i = 0; i < sizeof(inbuf); i++)
	{
		if(!(i%8))
		{
			printf("\n");
		}
		printf("0x%08x ", inbuf[i]);
	}
	printf("\n");

	printf("*****************************************************************************\n");
	memset(encrypt_out,0, sizeof(encrypt_out));
	memset(decrypt_out,0, sizeof(decrypt_out));
	encrypt(inbuf, aes_key, encrypt_out);
	printf("inbuf result:%s, len:%d\n", inbuf,sizeof(inbuf));
 	for(i = 0; i < sizeof(inbuf); i++)
	{
		if(!(i%8))
		{
			printf("\n");
		}
		printf("0x%08x ", inbuf[i]);
	}
	printf("\n");

	printf("encrypt result:%s, len:%d\n", encrypt_out,sizeof(encrypt_out));
	for(i = 0; i < sizeof(encrypt_out); i++)
	{
		if(!(i%8))
		{
			printf("\n");
		}
		printf("0x%08x ", encrypt_out[i]);
	}
	printf("\n");
    
	decrypt(encrypt_out, aes_key, decrypt_out);
	printf("decrypt result:%s, len:%d\n", decrypt_out,sizeof(decrypt_out));
 	for(i = 0; i < sizeof(decrypt_out); i++)
	{
		if(!(i%8))
		{
			printf("\n");
		}
		printf("0x%08x ", decrypt_out[i]);
	}
	printf("\n");

	printf("inbuf result:%s, len:%d\n", inbuf,sizeof(inbuf));
 	for(i = 0; i < sizeof(inbuf); i++)
	{
		if(!(i%8))
		{
			printf("\n");
		}
		printf("0x%08x ", inbuf[i]);
	}
	printf("\n");

    return 0;
}*/




/*
#define UPDATE_FILE_NM "./ptv1update.dat.aes" //被加密文件的路径
#define RELEASE_FILE_NM "./ptv1update.dat"   //解密之后文件的路径
#define AES_MALLOC_SZ 0x100000					//每一次以一M为整体进行解密
#define CELL_SZ 16											//AES解密算法以每16个字节进行加密解密
#define CELL_NU 65536                   //CELL_NU=AES_MALLOC_SZ/CELL_SZ


int main()
{
	unsigned char encrypt_out[16];//被加密的16个字节数据
	unsigned char decrypt_out[16]; 		//解密之后的16个字节数据
	unsigned char aes_key[32];				//密钥长度选择为32个字节，256位
	int i;
	char *key = "01234567890123456789012345678901"; //密钥为：

	int rd_encrypt_fd,wr_decrypt_fd;
	int flen;//加密文件长度
	int iRet;

	unsigned char *prd;
	unsigned char *pwr;

	memset(aes_key,0 ,sizeof(aes_key));
	memcpy(aes_key, key, sizeof(aes_key));

	rd_encrypt_fd = open(UPDATE_FILE_NM, O_RDONLY);
	if(-1 == rd_encrypt_fd)
	{
		printf(" no such file\n");
		return -1;
	}
	flen = lseek(rd_encrypt_fd, 0, SEEK_END);
	if(-1 == flen)
	{
		printf("seek end err\n");
		iRet = -1;
		goto OPRRDFILEERR;
	}
	iRet = lseek(rd_encrypt_fd, 0, SEEK_SET);
	if(-1 == iRet)
	{
		printf("seek begin err\n");
		iRet = -1;
		goto OPRRDFILEERR;
	}
	wr_decrypt_fd = open(RELEASE_FILE_NM, O_RDWR|O_CREAT, 0777);
	if(-1 == wr_decrypt_fd)
	{
		printf("open file for write err\n");
		iRet = -1;
		goto OPRRDFILEERR;
	}
	
	prd = (unsigned char*)malloc(AES_MALLOC_SZ);
	pwr = (unsigned char*)malloc(AES_MALLOC_SZ);
	if(NULL == prd || NULL == pwr)
	{
		printf("malloc err!");
		iRet = -1;
		goto OPRWRFILEERR;
	}


	printf("---------> filelen: %d\n",flen);
	while(flen >= AES_MALLOC_SZ)//AES_MALLOC_SZ=1M
	{
		iRet = read(rd_encrypt_fd, prd, AES_MALLOC_SZ);
		if(AES_MALLOC_SZ != iRet)
		{
			printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		for(i = 0; i < CELL_NU; i++)
		{
			memset(decrypt_out,0, sizeof(decrypt_out));
			decrypt(prd + i* CELL_SZ, aes_key, decrypt_out);
			memcpy(pwr + i* CELL_SZ, decrypt_out, sizeof(decrypt_out));
		}
		iRet = write(wr_decrypt_fd, pwr, AES_MALLOC_SZ);
		if(AES_MALLOC_SZ != iRet)
		{
			printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		flen -= AES_MALLOC_SZ;
	} 
	if(flen)
	{
		int last_nu = flen/CELL_SZ;	
		int last_sz = flen%CELL_SZ;	//always 0

		printf("i am here\n");
		iRet = read(rd_encrypt_fd, prd, flen);
		if(flen != iRet)
		{
			printf("read err !\n");
			iRet = -1;
			goto GENERALERR;
		}
		for(i = 0; i < last_nu; i++)
		{
			memset(decrypt_out,0, sizeof(decrypt_out));
			decrypt(prd + i* CELL_SZ, aes_key, decrypt_out);
			memcpy(pwr + i* CELL_SZ, decrypt_out, sizeof(decrypt_out));
		}
		if(last_sz)//这里不执行
		{
			memset(encrypt_out, 0, sizeof(encrypt_out));
			memcpy(encrypt_out, prd + i*CELL_SZ, last_sz);
			decrypt(encrypt_out, aes_key, decrypt_out);
			memcpy(pwr + i* CELL_SZ, decrypt_out, sizeof(decrypt_out));

			flen = flen - last_sz + CELL_SZ;
		}
		iRet = write(wr_decrypt_fd, pwr, flen);
		if(flen != iRet)
		{
			printf("write err !\n");
			iRet = -1;
			goto GENERALERR;
		}

	}

	printf("over ..........\n");
GENERALERR:
	free(prd);
	free(pwr);
OPRWRFILEERR:
	close(wr_decrypt_fd);
OPRRDFILEERR:
	close(rd_encrypt_fd);
	return iRet;
}*/
