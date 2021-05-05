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

Scheduler::Scheduler():_head(NULL),_tail(NULL),_size(0) {
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
