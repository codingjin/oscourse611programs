/*
 File: scheduler.C
 
 Author: Jin Huang
 Date  : 03/25/2021
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

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
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler()
 : _head(NULL),_tail(NULL),_size(0),_whead(NULL),_wtail(NULL),_wsize(0)
{
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {

	
	if (Machine::interrupts_enabled())
		Machine::disable_interrupts();
	

	if (_size) {

		Thread *t = _head->t;
		Node *tmp = _head;
		if (_size == 1) {
			_head = NULL;
			_tail = NULL;
		}else {
			_head = _head->next;
			_head->prev = NULL;
		}
		--_size;
		delete tmp;
		Machine::enable_interrupts();
		Thread::dispatch_to(t);
		//Console::puts("inside yield() after dispatch_to\n");
	}
	
	if (!Machine::interrupts_enabled())
		Machine::enable_interrupts();

}

void Scheduler::resume(Thread * _thread) {
	add(_thread);
}

void Scheduler::add(Thread * _thread) {
	assert(_thread != NULL);

	if (Machine::interrupts_enabled())
		Machine::disable_interrupts();

	Node *n = new Node();
	if (!n) {
		Console::puts("new Node() failed at Scheduler::add()!\n");
		return;
	}
	n->t = _thread;

	if (_size) {
		if (_size == 1) {
			n->prev = _head;
			n->next = NULL;
			_head->next = n;
			_tail = n;
		}else {
			n->next = NULL;
			n->prev = _tail;
			_tail->next = n;
			_tail = n;
		}
	}else {
		n->prev = NULL;
		n->next = NULL;
		_head = n;
		_tail = n;
	}

	++_size;

	if (!Machine::interrupts_enabled())
		Machine::enable_interrupts();

}

void Scheduler::terminate(Thread * _thread) {

	if (Machine::interrupts_enabled())
		Machine::disable_interrupts();


	if (_thread && _size) {
		Node * current = _head;
		while (current) {
			if (current->t == _thread)	break;
			current = current->next;
		}

		if (current) {
			
			if (_size == 1) {
				delete current;
				_head = NULL;
				_tail = NULL;
			}else if (_size == 2) {
				if (current == _head) {
					_head = _tail;
					_head->prev = NULL;
				}else { // current == _tail
					_tail = _head;	
					_head->next = NULL;
				}
				delete current;	
			}else {
				if (current == _head) {
					_head = _head->next;
					_head->prev = NULL;
				}else if (current == _tail) {
					_tail = _tail->prev;
					_tail->next = NULL;
				}else {
					current->prev->next = current->next;
					current->next->prev = current->prev;
				}
				delete current;
			}
			--_size;
		} // end if (current)
	}

	if (!Machine::interrupts_enabled())
		Machine::enable_interrupts();
	
}

void Scheduler::add_wait(Thread *_thread) {
	assert(_thread!=NULL && _wsize>=0);

	Node *n = new Node;
	assert(n != NULL);
	n->t = _thread;

	if (_wsize) {
		n->next = NULL;
		if (_wsize == 1) {
			n->prev = _whead;
			_whead->next = n;
		}else {
			n->prev = _wtail;
			_wtail->next = n;
		}
		_wtail = n;
		
	}else {
		n->prev = NULL;
		n->next = NULL;
		_whead = n;
		_wtail = n;
	}
	
	++_wsize;
}

void Scheduler::withdraw_wait() {
	if (_wsize > 0) {
		if (_wsize == 1) {
			delete _whead;
			_whead = NULL;
			_wtail = NULL;
		}else {
			Node *tmp = _wtail;
			_wtail = _wtail->prev;
			_wtail->next = NULL;
			delete tmp;
		}
		--_wsize;
	}
}

void Scheduler::switch_to_wait() {

	assert(_wsize>=0);
	if (_wsize==0)	return;

	Thread *t = _whead->t;

	// attain the first Node in wait queue
	Node *tmp = _whead;
	if (_wsize == 1) {
		_whead = NULL;
		_wtail = NULL;
	}else {
		_whead = _whead->next;
		_whead->prev = NULL;
	}
	--_wsize;
	delete tmp;

	Thread::dispatch_to(t);

}

void Scheduler::raw_add(Thread *_thread) {

	assert(_size>=0);
	Node *n = new Node;
	assert(n != NULL);
	n->t = _thread;
	n->next = NULL;

	if (_size) {
		n->prev = _tail;
		_tail->next = n;
	}else {
		n->prev = NULL;
		_head = n;
	}
	_tail = n;
	++_size;

}

int Scheduler::wait_size() {
	return _wsize;
}


