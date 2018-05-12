//Module name: libbps
//Author: Alcaro
//Date: November 7, 2015
//Licence: GPL v3.0 or higher

#include "libbps.h"

#include <stdlib.h>//malloc, realloc, free
#include <string.h>//memcpy, memset
#include <stdint.h>//uint8_t, uint32_t
#include "crc32.h"//crc32

static uint32_t read32(uint8_t * ptr)
{
	uint32_t out;
	out =ptr[0];
	out|=ptr[1]<<8;
	out|=ptr[2]<<16;
	out|=ptr[3]<<24;
	return out;
}

enum { SourceRead, TargetRead, SourceCopy, TargetCopy };

static bool try_add(size_t* a, size_t b)
{
	if (SIZE_MAX-*a < b) return false;
	*a+=b;
	return true;
}

static bool try_shift(size_t* a, size_t b)
{
	if (SIZE_MAX>>b < *a) return false;
	*a<<=b;
	return true;
}

static bool decodenum(const uint8_t* ptr, size_t* out)
{
	*out=0;
   uint32_t shift=0;
	while (true)
	{
		uint8_t next=*ptr++;
		size_t addthis=(next&0x7F);
		if (shift) addthis++;
		if (!try_shift(&addthis, shift)) return false;
		// unchecked because if it was shifted, the lowest bit is zero, and if not, it's <=0x7F.
		if (!try_add(out, addthis)) return false;
		if (next&0x80) return true;
		shift+=7;
	}
}

#define error(which) do { error=which; goto exit; } while(0)
#define assert_sum(a,b) do { if (SIZE_MAX-(a)<(b)) error(bps_too_big); } while(0)
#define assert_shift(a,b) do { if (SIZE_MAX>>(b)<(a)) error(bps_too_big); } while(0)
enum bpserror bps_apply(struct mem patch, struct mem in, struct mem * out, struct mem * metadata, bool accept_wrong_input)
{
	enum bpserror error = bps_ok;
	out->len=0;
	out->ptr=NULL;
	if (metadata)
	{
		metadata->len=0;
		metadata->ptr=NULL;
	}
	if (patch.len<4+3+12) return bps_broken;
	
	if (true)
	{
#define read8() (*(patchat++))
#define decodeto(var) \
				do { \
					if (!decodenum(patchat, &var)) error(bps_too_big); \
				} while(false)
#define write8(byte) (*(outat++)=byte)
		
		const uint8_t * patchat=patch.ptr;
		const uint8_t * patchend=patch.ptr+patch.len-12;
		
		if (read8()!='B') error(bps_broken);
		if (read8()!='P') error(bps_broken);
		if (read8()!='S') error(bps_broken);
		if (read8()!='1') error(bps_broken);
		
		uint32_t crc_in_e = read32(patch.ptr+patch.len-12);
		uint32_t crc_out_e = read32(patch.ptr+patch.len-8);
		uint32_t crc_patch_e = read32(patch.ptr+patch.len-4);
		
		uint32_t crc_in_a = crc32(in.ptr, in.len);
		uint32_t crc_patch_a = crc32(patch.ptr, patch.len-4);
		
		if (crc_patch_a != crc_patch_e) error(bps_broken);
		
		size_t inlen;
		decodeto(inlen);
		
		size_t outlen;
		decodeto(outlen);
		
		if (inlen!=in.len || crc_in_a!=crc_in_e)
		{
			if (in.len==outlen && crc_in_a==crc_out_e) error=bps_to_output;
			else error=bps_not_this;
			if (!accept_wrong_input) goto exit;
		}
		
		out->len=outlen;
		out->ptr=(uint8_t*)malloc(outlen);
		
		const uint8_t * instart=in.ptr;
		const uint8_t * inreadat=in.ptr;
		const uint8_t * inend=in.ptr+in.len;
		
		uint8_t * outstart=out->ptr;
		uint8_t * outreadat=out->ptr;
		uint8_t * outat=out->ptr;
		uint8_t * outend=out->ptr+out->len;
		
		size_t metadatalen;
		decodeto(metadatalen);
		
		if (metadata && metadatalen)
		{
			metadata->len=metadatalen;
			metadata->ptr=(uint8_t*)malloc(metadatalen+1);
			for (size_t i=0;i<metadatalen;i++) metadata->ptr[i]=read8();
			metadata->ptr[metadatalen]='\0';//just to be on the safe side - that metadata is assumed to be text, might as well terminate it
		}
		else
		{
			for (size_t i=0;i<metadatalen;i++) (void)read8();
		}
		
		while (patchat<patchend)
		{
			size_t thisinstr;
			decodeto(thisinstr);
			size_t length=(thisinstr>>2)+1;
			int action=(thisinstr&3);
			if (outat+length>outend) error(bps_broken);
			
			switch (action)
			{
				case SourceRead:
				{
					if (outat-outstart+length > in.len) error(bps_broken);
					for (size_t i=0;i<length;i++)
					{
						size_t pos = outat-outstart; // don't inline, write8 changes outat
						write8(instart[pos]);
					}
				}
				break;
				case TargetRead:
				{
					if (patchat+length>patchend) error(bps_broken);
					for (size_t i=0;i<length;i++) write8(read8());
				}
				break;
				case SourceCopy:
				{
					size_t encodeddistance;
					decodeto(encodeddistance);
					size_t distance=encodeddistance>>1;
					if ((encodeddistance&1)==0) inreadat+=distance;
					else inreadat-=distance;
					
					if (inreadat<instart || inreadat+length>inend) error(bps_broken);
					for (size_t i=0;i<length;i++) write8(*inreadat++);
				}
				break;
				case TargetCopy:
				{
					size_t encodeddistance;
					decodeto(encodeddistance);
					size_t distance=encodeddistance>>1;
					if ((encodeddistance&1)==0) outreadat+=distance;
					else outreadat-=distance;
					
					if (outreadat<outstart || outreadat>=outat || outreadat+length>outend) error(bps_broken);
					for (size_t i=0;i<length;i++) write8(*outreadat++);
				}
				break;
			}
		}
		if (patchat!=patchend) error(bps_broken);
		if (outat!=outend) error(bps_broken);
		
		uint32_t crc_out_a = crc32(out->ptr, out->len);
		
		if (crc_out_a!=crc_out_e)
		{
			error=bps_not_this;
			if (!accept_wrong_input) goto exit;
		}
		return error;
#undef read8
#undef decodeto
#undef write8
	}
	
exit:
	free(out->ptr);
	out->len=0;
	out->ptr=NULL;
	if (metadata)
	{
		free(metadata->ptr);
		metadata->len=0;
		metadata->ptr=NULL;
	}
	return error;
}



#define write(val) \
			do { \
				out[outlen++]=(val); \
				if (outlen==outbuflen) \
				{ \
					outbuflen*=2; \
					out=(uint8_t*)realloc(out, outbuflen); \
				} \
			} while(0)
#define write32(val) \
			do { \
				uint32_t tmp=(val); \
				write(tmp); \
				write(tmp>>8); \
				write(tmp>>16); \
				write(tmp>>24); \
			} while(0)
#define writenum(val) \
			do { \
				size_t tmpval=(val); \
				while (true) \
				{ \
					uint8_t tmpbyte=(tmpval&0x7F); \
					tmpval>>=7; \
					if (!tmpval) \
					{ \
						write(tmpbyte|0x80); \
						break; \
					} \
					write(tmpbyte); \
					tmpval--; \
				} \
			} while(0)

enum bpserror bps_create_linear(struct mem sourcemem, struct mem targetmem, struct mem metadata, struct mem * patchmem)
{
	if (sourcemem.len>=(SIZE_MAX>>2) - 16) return bps_too_big;//the 16 is just to be on the safe side, I don't think it's needed.
	if (targetmem.len>=(SIZE_MAX>>2) - 16) return bps_too_big;
	
	const uint8_t * source=sourcemem.ptr;
	const uint8_t * sourceend=sourcemem.ptr+sourcemem.len;
	if (sourcemem.len>targetmem.len) sourceend=sourcemem.ptr+targetmem.len;
	const uint8_t * targetbegin=targetmem.ptr;
	const uint8_t * target=targetmem.ptr;
	const uint8_t * targetend=targetmem.ptr+targetmem.len;
	
	const uint8_t * targetcopypos=targetbegin;
	
	size_t outbuflen=4096;
	uint8_t * out=(uint8_t*)malloc(outbuflen);
	size_t outlen=0;
	write('B');
	write('P');
	write('S');
	write('1');
	writenum(sourcemem.len);
	writenum(targetmem.len);
	writenum(metadata.len);
	for (size_t i=0;i<metadata.len;i++) write(metadata.ptr[i]);
	
	size_t mainContentPos=outlen;
	
	const uint8_t * lastknownchange=targetbegin;
	while (target<targetend)
	{
		size_t numunchanged=0;
		while (source+numunchanged<sourceend && source[numunchanged]==target[numunchanged]) numunchanged++;
		if (numunchanged>1 || numunchanged == (uintptr_t)(targetend-target))
		{
			//assert_shift((numunchanged-1), 2);
			writenum((numunchanged-1)<<2 | 0);//SourceRead
			source+=numunchanged;
			target+=numunchanged;
		}
		
		size_t numchanged=0;
		if (lastknownchange>target) numchanged=lastknownchange-target;
		while ((source+numchanged>=sourceend ||
		        source[numchanged]!=target[numchanged] ||
		        source[numchanged+1]!=target[numchanged+1] ||
		        source[numchanged+2]!=target[numchanged+2]) &&
		       target+numchanged<targetend)
		{
			numchanged++;
			if (source+numchanged>=sourceend) numchanged=targetend-target;
		}
		lastknownchange=target+numchanged;
		if (numchanged)
		{
			//assert_shift((numchanged-1), 2);
			size_t rle1start=(target==targetbegin);
			while (true)
			{
				if (
					target[rle1start-1]==target[rle1start+0] &&
					target[rle1start+0]==target[rle1start+1] &&
					target[rle1start+1]==target[rle1start+2] &&
					target[rle1start+2]==target[rle1start+3])
				{
					numchanged=rle1start;
					break;
				}
				if (
					target[rle1start-2]==target[rle1start+0] &&
					target[rle1start-1]==target[rle1start+1] &&
					target[rle1start+0]==target[rle1start+2] &&
					target[rle1start+1]==target[rle1start+3] &&
					target[rle1start+2]==target[rle1start+4])
				{
					numchanged=rle1start;
					break;
				}
				if (rle1start+3>=numchanged) break;
				rle1start++;
			}
			if (numchanged)
			{
				writenum((numchanged-1)<<2 | TargetRead);
				for (size_t i=0;i<numchanged;i++)
				{
					write(target[i]);
				}
				source+=numchanged;
				target+=numchanged;
			}
			if (target[-2]==target[0] && target[-1]==target[1] && target[0]==target[2])
			{
				//two-byte RLE
				size_t rlelen=0;
				while (target+rlelen<targetend && target[0]==target[rlelen+0] && target[1]==target[rlelen+1]) rlelen+=2;
				writenum((rlelen-1)<<2 | TargetCopy);
				writenum((target-targetcopypos-2)<<1);
				source+=rlelen;
				target+=rlelen;
				targetcopypos=target-2;
			}
			else if (target[-1]==target[0] && target[0]==target[1])
			{
				//one-byte RLE
				size_t rlelen=0;
				while (target+rlelen<targetend && target[0]==target[rlelen]) rlelen++;
				writenum((rlelen-1)<<2 | TargetCopy);
				writenum((target-targetcopypos-1)<<1);
				source+=rlelen;
				target+=rlelen;
				targetcopypos=target-1;
			}
		}
	}
	
	write32(crc32(sourcemem.ptr, sourcemem.len));
	write32(crc32(targetmem.ptr, targetmem.len));
	write32(crc32(out, outlen));
	
	patchmem->ptr=out;
	patchmem->len=outlen;
	
	//while this may look like it can be fooled by a patch containing one of any other command, it
	//  can't, because the ones that aren't SourceRead requires an argument.
	size_t i;
	for (i=mainContentPos;(out[i]&0x80)==0x00;i++) {}
	if (i==outlen-12-1) return bps_identical;
	
	return bps_ok;
}

#undef write_nocrc
#undef write
#undef writenum

void bps_free(struct mem mem)
{
	free(mem.ptr);
}
#undef error
