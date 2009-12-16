/*
* CBC Mode
* (C) 1999-2007 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/cbc.h>
#include <botan/internal/xor_buf.h>
#include <algorithm>

namespace Botan {

/*
* CBC Encryption Constructor
*/
CBC_Encryption::CBC_Encryption(BlockCipher* ciph,
                               BlockCipherModePaddingMethod* pad) :
   BlockCipherMode(ciph, "CBC", ciph->BLOCK_SIZE),
   padder(pad)
   {
   if(!padder->valid_blocksize(BLOCK_SIZE))
      throw Invalid_Block_Size(name(), padder->name());
   }

/*
* CBC Encryption Constructor
*/
CBC_Encryption::CBC_Encryption(BlockCipher* ciph,
                               BlockCipherModePaddingMethod* pad,
                               const SymmetricKey& key,
                               const InitializationVector& iv) :
   BlockCipherMode(ciph, "CBC", ciph->BLOCK_SIZE),
   padder(pad)
   {
   if(!padder->valid_blocksize(BLOCK_SIZE))
      throw Invalid_Block_Size(name(), padder->name());
   set_key(key);
   set_iv(iv);
   }

/*
* Encrypt in CBC mode
*/
void CBC_Encryption::write(const byte input[], u32bit length)
   {
   while(length)
      {
      u32bit xored = std::min(BLOCK_SIZE - position, length);
      xor_buf(state + position, input, xored);
      input += xored;
      length -= xored;
      position += xored;
      if(position == BLOCK_SIZE)
         {
         cipher->encrypt(state);
         send(state, BLOCK_SIZE);
         position = 0;
         }
      }
   }

/*
* Finish encrypting in CBC mode
*/
void CBC_Encryption::end_msg()
   {
   SecureVector<byte> padding(BLOCK_SIZE);
   padder->pad(padding, padding.size(), position);
   write(padding, padder->pad_bytes(BLOCK_SIZE, position));
   if(position != 0)
      throw Exception(name() + ": Did not pad to full blocksize");
   }

/*
* Return a CBC mode name
*/
std::string CBC_Encryption::name() const
   {
   return (cipher->name() + "/" + mode_name + "/" + padder->name());
   }

/*
* CBC Decryption Constructor
*/
CBC_Decryption::CBC_Decryption(BlockCipher* ciph,
                               BlockCipherModePaddingMethod* pad) :
   BlockCipherMode(ciph, "CBC", ciph->BLOCK_SIZE),
   padder(pad)
   {
   if(!padder->valid_blocksize(BLOCK_SIZE))
      throw Invalid_Block_Size(name(), padder->name());
   temp.resize(BLOCK_SIZE);
   }

/*
* CBC Decryption Constructor
*/
CBC_Decryption::CBC_Decryption(BlockCipher* ciph,
                               BlockCipherModePaddingMethod* pad,
                               const SymmetricKey& key,
                               const InitializationVector& iv) :
   BlockCipherMode(ciph, "CBC", ciph->BLOCK_SIZE),
   padder(pad)
   {
   if(!padder->valid_blocksize(BLOCK_SIZE))
      throw Invalid_Block_Size(name(), padder->name());
   temp.resize(BLOCK_SIZE);
   set_key(key);
   set_iv(iv);
   }

/*
* Decrypt in CBC mode
*/
void CBC_Decryption::write(const byte input[], u32bit length)
   {
   while(length)
      {
      if(position == BLOCK_SIZE)
         {
         cipher->decrypt(buffer, temp);
         xor_buf(temp, state, BLOCK_SIZE);
         send(temp, BLOCK_SIZE);
         state = buffer;
         position = 0;
         }
      u32bit added = std::min(BLOCK_SIZE - position, length);
      buffer.copy(position, input, added);
      input += added;
      length -= added;
      position += added;
      }
   }

/*
* Finish decrypting in CBC mode
*/
void CBC_Decryption::end_msg()
   {
   if(position != BLOCK_SIZE)
      throw Decoding_Error(name());
   cipher->decrypt(buffer, temp);
   xor_buf(temp, state, BLOCK_SIZE);
   send(temp, padder->unpad(temp, BLOCK_SIZE));
   state = buffer;
   position = 0;
   }

/*
* Return a CBC mode name
*/
std::string CBC_Decryption::name() const
   {
   return (cipher->name() + "/" + mode_name + "/" + padder->name());
   }

}
