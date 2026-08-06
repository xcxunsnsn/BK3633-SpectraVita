/**
****************************************************************************************
*
* @file bt_util_sp.h
*
* @brief Link Manager simple pairing
*
* Copyright (C) RivieraWaves 2009-2015
*
****************************************************************************************
*/
#ifndef BT_UTIL_SP_H_
#define BT_UTIL_SP_H_
/**
****************************************************************************************
* @defgroup BTUTILSP SP Computation
* @ingroup LM
* @brief LM module for simple pairing algorithms and mathematical computations.
* @{
****************************************************************************************
*/

#include <stdint.h>
#include "co_bt.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define LM_OPER_SUCCESS                     0x00
#define LM_OPER_PENDING                     0x01
#define LM_OPER_FAILED                      0x02

#define LM_SP_WORD_BITS4        16

#define LM_SP_WORD_MASK2        0xffffffff
#define LM_SP_WORD_MASK2l       0x0000ffff
#define LM_SP_WORD_MASK2h1      0xffff8000
#define LM_SP_WORD_MASK2h       0xffff0000
#define LM_SP_WORD_TBIT         0x80000000

#define LM_SPLBITS(a)           ((a) & LM_SP_WORD_MASK2l)
#define LM_SPHBITS(a)           (((a) >> LM_SP_WORD_BITS4)& LM_SP_WORD_MASK2l)
#define LM_SPL2HBITS(a)         (((a) << LM_SP_WORD_BITS4)& LM_SP_WORD_MASK2)

#define LM_SP_NUM_192_SIGN_BIT  0x80000000
#define LM_SP_NUM_192_BYTES     0x06
#define LM_SP_NUM_192_BITS      32

#define LM_SP_TYPE_NUM_192      1
#define LM_SP_TYPE_NUM_384      2

#define LM_SP_WNAF_WINDOW           0x03
#define LM_SP_WNAF_MSB_BIT          (1<<LM_SP_WNAF_WINDOW)
#define LM_SP_WNAF_NEXT_DIGIT_BIT   (LM_SP_WNAF_MSB_BIT<<1)
#define LM_SP_WNAF_DIGIT_MASK       (LM_SP_WNAF_NEXT_DIGIT_BIT-1)

#define LM_SP_P192_PRE_COMP_POINTS  4

/*
 * NEW TYPE
 * ***************************************************************************************
 */

typedef uint8_t u_int8;
typedef uint32_t u_int32;

/// Simple pairing 192
typedef struct _lm_sp_num_192_
{
    uint32_t Neg;
    uint32_t Number[6];
}
lm_sp_num_192;

/// Simple pairing 384
typedef struct _lm_sp_num_384_
{
    uint32_t Neg;
    uint32_t Number[12];
}
lm_sp_num_384;

/// Simple pairing P192
typedef struct _lm_sp_p192_point_
{
    lm_sp_num_192 X;
    lm_sp_num_192 Y;
    lm_sp_num_192 Z;
}
lm_sp_p192_point;

/// Simple pairing P256
typedef struct _lm_sp_p256_point_
{
    uint8_t X[32];
    uint8_t Y[32];
}
lm_sp_p256_point;

/// P-192 curve data
typedef struct _lm_sp_p192_group_data_
{
    lm_sp_p192_point    G;
    lm_sp_num_192       A;
}
lm_sp_p192_group_data;

/// Simple pairing P192 Computation Global Data
typedef struct _lm_sp_p192_comp_global_data_
{
    lm_sp_p192_point     Pre_comp[LM_SP_P192_PRE_COMP_POINTS];
    lm_sp_p192_point     RemotePublicKey;
    lm_sp_p192_point     DH_Key;
    signed char          wnaf[196];
    uint32_t             wnaf_len;
    uint8_t              Loop_ctr;
    uint8_t              Result_at_infinity;
    uint8_t              Result_inverted;
}
lm_sp_p192_comp_global_data;

/// Simple pairing P256 Computation Global Data
typedef struct _lm_sp_p256_comp_global_data_
{
    lm_sp_p256_point     RemotePublicKey;
    lm_sp_p256_point     DH_Key;
}
lm_sp_p256_comp_global_data;

/*
 * FUNCTION DECLARATION
 * ***************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief This function calculates the SHA hash for the given input block.
 *
 * @param[in] Input_Message    The pointer to the message block for which hash is to be calculated
 * @param[in] SHA_256_Hash     Hash Value calculated
 * @param[in] Length           Length of the Input block
 ****************************************************************************************
 */
void lm_sp_sha256_calculate(uint8_t* Input_Message, uint32_t* SHA_256_Hash, uint32_t Length);

/**
 ****************************************************************************************
 * @brief This function checks if the input is zero
 *
 * @param[in] a      The input
 * @param[in] Type   Type of the input
 *
 * @return non zero if number is zero else returns 0
 *
 ****************************************************************************************
 */
int lm_n_is_zero(void* a, uint32_t type);

/**
 ****************************************************************************************
 * @brief This function stores the one on a given number.
 *
 * @param[in] a     The input
 * @param[in] Type  Type of the input
 ****************************************************************************************
 */
void lm_sp_n_one(void* a, uint32_t type);

/**
 ****************************************************************************************
 * @brief This function returns converts the number into naf form with window
 *        size of 4
 *
 * @param[in] wnaf     returns wnaf form
 * @param[in] input    the input num to be converted
 * @param[in] ret_len  the length of the wnaf form number
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
int lm_sp_n192_convert_wnaf(signed char *r, lm_sp_num_192 *scalar, uint32_t *ret_len);

/**
 ****************************************************************************************
 * @brief This function sets the input point to infinity
 *
 * @param[in] p     Input
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
int lm_sp_p192_point_to_inf(lm_sp_p192_point *p);

/**
 ****************************************************************************************
 * @brief This function converts the coordinate system from jacobian to affine
 *
 * @param[in] p      Input
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
int lm_sp_p192_point_jacobian_to_affine(lm_sp_p192_point *points);

/**
 ****************************************************************************************
 * @brief This function is used to convert the points from jacobian to affine
 *
 * @param[in] point  The array of points to be converted from Jacobian to affine
 *
 * @return non zero on success and vice versa
 *
 ****************************************************************************************
 */
int lm_sp_p192_points_jacobian_to_affine(lm_sp_p192_point *points);

/**
 ****************************************************************************************
 * @brief This function is used to precompute the points needed for scalar mul
 *
 * @param[in] r     The array for P192 pre computation
 *
 * @return non zero on success and vice versa
 ****************************************************************************************
 */
int lm_sp_pre_compute_points(lm_sp_p192_point *r);

/**
 ****************************************************************************************
 * @brief This function doubles the given p192 point
 *
 * @param[in] r     The result after point doubling
 * @param[in] a     The point to be doubled
 *
 * @return non zero on success and vice versa
 ****************************************************************************************
 */
int lm_sp_p192_dbl(lm_sp_p192_point *r, lm_sp_p192_point *a);

/**
 ****************************************************************************************
 * @brief This function adds two  p192 point
 *
 * @param[in] r     The result after point addition
 * @param[in] a     The point to be added
 * @param[in] b     The point to be added
 *
 * @return non zero on success and vice versa
 ****************************************************************************************
 */
int lm_sp_p192_add(lm_sp_p192_point *r, lm_sp_p192_point *a, lm_sp_p192_point *b);

/**
 ****************************************************************************************
 * @brief This function inverts the input p192 point
 *
 * @param[in] p    Input
 *
 * @return non zero on success and vice versa
 ****************************************************************************************
 */
int lm_sp_p192_invert(lm_sp_p192_point *p);

/**
 ****************************************************************************************
 * @brief This function generates a 16-bytes random number
 *
 * @param[out] number            Pointer to random number buffer
 ****************************************************************************************
 */
void lm_generate_rand_16bytes(struct byte16* number);

/**
 ****************************************************************************************
 * @brief This function is used to calculate the F1 Value.
 *
 * @param[in] LocRandN         Pointer to ltk structure local random number
 * @param[in] Commitment       Pointer to ltk structure commitment value
 * @param[in] Number           Numerical value
 * @param[in] Role             Device role
 * @param[in] lm_sp_p192_dhkey_data   Buffer with SP informations
 ****************************************************************************************
 */
void lm_f1(struct byte16* LocRandN, struct byte16* Commitment, uint8_t Number, uint8_t Role, lm_sp_p192_comp_global_data* lm_sp_p192_dhkey_data);

/**
 ****************************************************************************************
 * @brief This function is used to calculate the F1 Value for OOB.
 *
 * @param[in] Nonce               Pointer to ltk structure nonce value
 * @param[in] Commitment          Pointer to ltk structure commitment value
 * @param[in] Number              Numerical value
 * @param[in] IsPublicKeyLocal    Check if local public key
 * @param[in] lm_sp_p192_dhkey_data   Buffer with SP informations
 ****************************************************************************************
 */
void lm_oob_f1(struct byte16* Nonce, struct byte16* Commitment, uint8_t Number,
                  uint8_t IsPublicKeyLocal, lm_sp_p192_comp_global_data* lm_sp_p192_dhkey_data);
/**
 ****************************************************************************************
 * @brief This function is the G function.
 *
 * @param[in] Na                Pointer to ltk structure Na
 * @param[in] Nb                Pointer to ltk structure Nb
 * @param[in] Role              Role
 * @param[in] lm_sp_p192_dhkey_data   Buffer with SP informations
 * @param[in] UserConfirmNum    User confirmation number
 ****************************************************************************************
 */
void lm_g(struct byte16* Na, struct byte16* Nb, uint8_t Role, lm_sp_p192_comp_global_data* lm_sp_p192_dhkey_data, uint32_t* UserConfirmNum);
/**
 ****************************************************************************************
 * @brief This function is used to calculate the F3 Value.
 *
 * @param[in] N1                        N1 16-byte length value
 * @param[in] N2                        N2 16-byte length value
 * @param[in] R                         Randomizer
 * @param[in] IOCAP                     IO capability
 * @param[in] B                         BD address
 * @param[in] lm_sp_p192_dhkey_data   Buffer with SP informations
 * @param[in] CheckValue                Value check
 *
 * @return Status
 ****************************************************************************************
 */
void lm_f3(struct byte16* N1,          struct byte16* N2, struct randomizer* R,
              struct io_capability IOCAP, struct bd_addr* A, struct bd_addr* B,
              lm_sp_p192_comp_global_data* lm_sp_p192_dhkey_data,   struct byte16* CheckValue);
/**
 ****************************************************************************************
 * @brief This function is used to calculate the F2 Value.
 *
 * @param[in] N1                 N1 16-byte length value
 * @param[in] N2                 N2 16-byte length value
 * @param[in] A                  BD address A
 * @param[in] B                  BD address B
 * @param[in] lm_sp_p192_dhkey_data   Buffer with SP informations
 * @param[in] LinkKey            Link key LTK structure
 ****************************************************************************************
 */
void lm_f2(struct byte16* N1, struct byte16* N2, struct bd_addr* A, struct bd_addr* B,
              lm_sp_p192_comp_global_data* lm_sp_p192_dhkey_data, struct ltk* LinkKey);

/**
 ****************************************************************************************
 * @brief Initialize Diffie Hellman key calculation
 *
 * @param[in] BufferIndex          Buffer index for sp
 *
 * @return Status
 *
 ****************************************************************************************
 */
uint8_t lm_dhkey_calc_init(lm_sp_p192_comp_global_data* lm_sp_p192_dhkey_data);
/**
 ****************************************************************************************
 * @brief This function compares DHkeys.
 *
 * @param[in] BufferIndex          Buffer index for sp
 *
 * @return Status
 *
 ****************************************************************************************
 */
uint8_t lm_dhkey_compare(lm_sp_p192_comp_global_data* lm_sp_p192_dhkey_data);

/**
 ****************************************************************************************
 * @brief This function is used to calculate the F1 Value for P-256.
 *
 * @param[in] U       pointer to 32 byte value U (see BT Spec)
 * @param[in] V       pointer to 32 byte value V (see BT Spec)
 * @param[in] X       pointer to 16 byte value X (see BT Spec)
 * @param[in] Z       pointer to single byte Z (see BT Spec)
 *
 * @param[out] Output  pointer to 16 byte result (see BT Spec)
 *
 ****************************************************************************************
 */
void F1_256(const u_int8 *U,const u_int8 *V,const u_int8 *X,const u_int8 *Z,u_int8* OutPut);


/**
 ****************************************************************************************
 * @brief This function is used to calculate the G value for P-256.
 *
 * g(U, V, X, Y) = SHA-256 (U || V || X || Y) / 2e32
 *
 * @param[in] U       pointer to 32 byte value U (see BT Spec)
 * @param[in] V       pointer to 32 byte value V (see BT Spec)
 * @param[in] X       pointer to 16 byte value X (see BT Spec)
 * @param[in] Y       pointer to 16 byte value Y (see BT Spec)
 *
 * @param[out] Output  pointer to 4 byte result (see BT Spec)
 ****************************************************************************************
 */
void G_256(const u_int8 *U,const u_int8 *V,const u_int8 *X,const u_int8 *Y,u_int32* OutPut);

/**
 ****************************************************************************************
 * @brief This function is used to calculate the F2 value for P-256.
 *
 * @param[in] W       pointer to 32 byte value W (see BT Spec)
 * @param[in] N1      pointer to 16 byte value N1 (see BT Spec)
 * @param[in] N2      pointer to 16 byte value N2 (see BT Spec)
 * @param[in] KeyId   pointer to 4 Byte KeyId(see BT Spec)
 * @param[in] A1      pointer to 6 Address value A1 (see BT Spec)
 * @param[in] A2      pointer to 6 Address value A2 (see BT Spec)
 *
 * @param[out] Output  pointer to 16 byte result (see BT Spec)
 ****************************************************************************************
 */
void F2_256(const u_int8 *W,const u_int8 *N1, const u_int8 *N2, const u_int8 *KeyId, const u_int8 *A1, const u_int8 *A2,u_int8* OutPut);

/**
 ****************************************************************************************
 * @brief This function is used to calculate the F3 value for P-256.
 *
 * @param[in] W       pointer to 32 byte value W (see BT Spec)
 * @param[in] N1      pointer to 16 byte value N1 (see BT Spec)
 * @param[in] N2      pointer to 16 byte value N2 (see BT Spec)
 * @param[in] R1      pointer to 16 byte value R1 (see BT Spec)
 * @param[in] IOcap   pointer to 4 Byte IO Capabilities (see BT Spec)
 * @param[in] A1      pointer to 6 Address value A1 (see BT Spec)
 * @param[in] A2      pointer to 6 Address value A2 (see BT Spec)
 *
 * @param[out] Output  pointer to 16 byte result (see BT Spec)
 ****************************************************************************************
 */
void F3_256(const u_int8 *W,const u_int8 *N1,const u_int8 *N2,const u_int8 *R, const u_int8 *IOcap,const u_int8 *A1,const u_int8 *A2, u_int8* OutPut);
/**
 ****************************************************************************************
 * @brief This function is used to calculate the H3 value for P-256.
 * The AES encryption key is generated using H3.
 *
 * h3(W, keyID, A1, A2, ACO) = HMAC-SHA-256T(KeyID || A1 || A2 || ACO) / 2^128
 *
 * @param[in] T       pointer to the 128 bit Bluetooth Link Key derived from f2
 * @param[in] A1      pointer to 6 Byte BD_ADDR of the master.
 * @param[in] A2      pointer to 6 Byte BD_ADDR of the slave.
 * @param[in] ACO     pointer to 8 byte ACO output from h5
 *
 * @param[out] pResult  pointer to 16 byte result (see BT Spec)
 ****************************************************************************************
 */

u_int8* H3(const u_int8* T,const u_int8* A1, const u_int8* A2, const u_int8* ACO, u_int8* pResult);


/**
 ****************************************************************************************
 * @brief This function is used to calculate the H4 value for P-256.
 * H4 determines the Device Authentication Key
 *
 * h4(W, keyID, A1, A2) = HMAC-SHA-256T(KeyID || A1 || A2) / 2^128
 *
 * @param[in] T       pointer to the 128 bit Bluetooth Link Key.
 * @param[in] A1      pointer to 6 Byte BD_ADDR of the master.
 * @param[in] A2      pointer to 6 Byte BD_ADDR of the slave.
 *
 * @param[out] pResult  pointer to 16 byte result (see BT Spec)
 *
 * NOTE :- The KeyId is fixed and thus not passed as a parameter.
 ****************************************************************************************
 */

u_int8* H4(const u_int8* T,const u_int8* A1, const u_int8* A2, u_int8* pResult);

/**
 ****************************************************************************************
 * @brief This function is used to calculate the H5 value for P-256.
 * H5 if used for the SRES and ACO calculation. The R1 and R2 are the AU_RANDs of the
 * Master and Slave (AU_RANDs and AU_RANDm)
 *
 *  h5(W, R1, R2) = HMAC-SHA-256T( R1 || R2) / 2^128
 *
 * @param[in] W       pointer to the 16 Byte Device Authentication Key.
 * @param[in] R1      pointer to 16 Byte AU_RAND.
 * @param[in] R2      pointer to 16 Byte AU_RAND.
 *
 * @param[out] pResult  pointer to 16 byte result (see BT Spec)
 *
 ****************************************************************************************
 */

u_int8* H5(const u_int8* W,const u_int8* R_1,const u_int8* R_2, u_int8* pResult);

/**
 ****************************************************************************************
 * @brief This is the HMAC funtion used my most of the SSP Crypto functions.
 *
 *  HMACk(m) = h((K|opad)||h((K|ipad)||m))
 *  where h is a cryptographic hash function, K is a secret key padded to the
 *  right with extra zeros to the block size of a the hash function.
 *
 * @param[in] Key          pointer to the Key.
 * @param[in] Key_Len      length of the Key in BITs .
 * @param[in] message      pointer to the Message.
 * @param[in] message_len  length of the message in BITs .
 *
 * returns :- a pointer to the 32 Byte HMAC output.
 *
 * NOTE :- The pointer to the HMAC output is actually a pointer to a static array.
 *
 ****************************************************************************************
 */

u_int8* HMAC(const u_int8* Key,const int Key_Len,const u_int8 *message,int message_len);

/**
 ****************************************************************************************
 * @brief This is a message concatenation function.
 *
 *  This function is rather STUPID - and should be removed/deprecated. Simpler to replace
 *  with memcpy
 *
 * @param[in] Message1          pointer to the first message.
 * @param[in] Message1_Len      length of the first message in BITs .
 * @param[in] Message2          pointer to the second  message.
 * @param[in] Message2_Len      length of the second message in BITs .
 *
 * @param[out] newConcatBuffer  pointer to concatenated result
 *
 * returns :- a pointer to the 32 Byte HMAC output.
 *
 * NOTE :- This function should be removed..
 *
 ****************************************************************************************
 */
u_int32 hashConcat(const u_int8* Message1,u_int32 Message1_Len,const u_int8* Message2, u_int32 Message2_Len,u_int8* newConcatBuffer);

/**
 ****************************************************************************************
 * @brief This is the SHA-256 function.
 *
 * This function determines HASH for a given message.
 *
 * @param[in] Message                  pointer to the message to be hashed
 * @param[in] MessageLengthInBits      length of the message in BITs.
 *
 * @param[out] Hash                    pointer to 32 byte resultant hash
 *
 * returns :- Length in Bits of the Hash (fixed at 256).
 *
 ****************************************************************************************
 */

u_int32 SHA_256(u_int8* Message,u_int32 MessageLengthInBits,u_int8* Hash);

///@} BTUTILSP

#endif // BT_UTIL_SP_H_
