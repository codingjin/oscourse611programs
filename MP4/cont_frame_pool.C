/*
 File: ContFramePool.C
 
 Author: Jin Huang
 Date  : 02/14/2021
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/
ContFramePool* ContFramePool::head = NULL;
ContFramePool* ContFramePool::tail = NULL;


unsigned char ContFramePool::getbit(unsigned char *bitmap, int findex)
{
	assert((findex>=frame_begin) && (findex<=frame_end));
	int foffset = findex - frame_begin;
	int n_byte = foffset / 8;
	int byte_offset = foffset % 8;
	int rshift = 7 - byte_offset;
	return (bitmap[n_byte]>>rshift)&1;
}

int ContFramePool::setbit(unsigned char *bitmap, int findex)
{
	assert((findex>=frame_begin) && (findex<=frame_end));
	unsigned char mask = 0x80;
	int foffset = findex - frame_begin;
	int n_byte = foffset / 8;
	int byte_offset = foffset % 8;
	int rshift = 7 - byte_offset;
	if ((bitmap[n_byte]>>rshift)&1)
		return 0;
	bitmap[n_byte] = bitmap[n_byte] ^ (mask>>byte_offset);
	return 1;
}

int ContFramePool::clearbit(unsigned char *bitmap, int findex)
{
	assert((findex>=frame_begin) && (findex<=frame_end));
	unsigned char mask = 0x80;
	int foffset = findex - frame_begin;
	int n_byte = foffset / 8;
	int byte_offset = foffset % 8;
	int rshift = 7 - byte_offset;
	if (((bitmap[n_byte]>>rshift)&1) == 0)
		return 0;
	bitmap[n_byte] = bitmap[n_byte] ^ (mask>>byte_offset);
	return 1;
}

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
	base_frame_no = _base_frame_no;
	assert(base_frame_no>=512 && base_frame_no<8192);
	n_frames = _n_frames;
	assert((n_frames>=1) && ((n_frames%8)==0) && (n_frames<=8192));
	info_frame_no = _info_frame_no;
	n_info_frames = _n_info_frames;
	if (info_frame_no == 0)	{
		//the management info for this pool should be stored INTERNALLY
		info_frame_no = base_frame_no;
		n_info_frames=1;
	}
	assert(n_info_frames>=1);
	frame_begin = base_frame_no;
	frame_end = base_frame_no + n_frames - 1;

	max_map_index = n_frames/8 - 1;
	//bitmap1 = (unsigned char *)(base_frame_no * FRAME_SIZE);
	bitmap1 = (unsigned char *)(info_frame_no * FRAME_SIZE);
	bitmap2 = (unsigned char *)(bitmap1 + MAX_MAPINDEX_BOUND);
	memset(bitmap1, 0xFF, max_map_index+1);
	memset(bitmap2, 0xFF, max_map_index+1);

	// assign the info_frame, if the management info is stored internally
	if (_n_info_frames == 0) {
		// set the first frame for used, for the info. the base_frame_no
		this->clearbit(bitmap1, base_frame_no);
		// set the head for info_frame. the base_frame_no
		this->clearbit(bitmap2, base_frame_no);
		--n_frames;
	}

	if (ContFramePool::head == NULL) {
		ContFramePool::head = this;
		ContFramePool::tail = this;
		this->prev = NULL;
		this->next = NULL;
	}else {
		ContFramePool::tail->next = this;
		this->prev = ContFramePool::tail;
		this->next = NULL;
		ContFramePool::tail = this;
	}

}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
	if (_n_frames > n_frames) {
		Console::puts("_n_frames=");Console::puti(_n_frames);
		Console::puts("\nn_frames=");Console::puti(n_frames);
		Console::puts("\n_n_frames > n_frames\n");
		return 0;
	}

	int findex = frame_begin;
	int try_frame_end = frame_end - _n_frames + 1;
	int flag = 0;
	while (findex <= try_frame_end) {
		flag = 0;
		int i=0;
		for (;i<_n_frames;++i) {
			if (this->getbit(bitmap1, findex+i) == 0) {
				flag = 1;
				break;
			}
		}
		if (flag)	findex = findex + i + 1;
		else	break;
	}
	// not enough continuous frames for allocation br
	if (flag == 1)	{
		Console::puts("not enough continuous frames\n");
		return 0;
	}

	// allocate now
	for (int i=0;i<_n_frames;++i)	this->clearbit(bitmap1, findex+i); // mark used in bitmap1
	
	// mark the head on bitmap2
	this->clearbit(bitmap2, findex);
	n_frames = n_frames - _n_frames;

	return findex;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
	assert((_base_frame_no>=frame_begin) && (_base_frame_no<=frame_end) && (_n_frames<=n_frames));
	int findex = _base_frame_no;
	for (;findex<_base_frame_no+_n_frames;++findex) {
		assert(this->clearbit(bitmap1, findex)==1);
	}
	// also mark the head on bitmap2
	this->clearbit(bitmap2, _base_frame_no);
	n_frames = n_frames - _n_frames;
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
	ContFramePool *p = ContFramePool::head;
	while (p != NULL) {
		int flag = 0;
		if (p->frame_begin<= _first_frame_no && p->frame_end>=_first_frame_no) {
			// _first_frame_no matches p, and _first_frame_no should be the head 
			assert(p->getbit(p->bitmap1, _first_frame_no)==0
					&& p->getbit(p->bitmap2, _first_frame_no)==0);

			flag = 1;
			// now begin to release frames
			int findex = _first_frame_no;

			// only release the head first
			p->setbit(p->bitmap1, findex);
			p->setbit(p->bitmap2, findex);
			p->n_frames++;
			findex++;

			for (;findex<=p->frame_end;++findex) {
				if (p->getbit(p->bitmap1, findex)==1 
					|| (p->getbit(p->bitmap1, findex)==0 
						&& p->getbit(p->bitmap2, findex)==0)) {
					break;
				}
				
				// release this frame
				p->setbit(p->bitmap1, findex);
				p->n_frames++;
			}

		}

		if (flag)	return; // release_frames finish
		else	p = p->next;
	}
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
	return _n_frames/8192 + (_n_frames%8192>0? 1:0);
}
