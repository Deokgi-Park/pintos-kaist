#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "lib/user/syscall.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
void check_address(void *addr);
static struct file *find_file_by_fd(int fd);
int add_file_to_fdt(struct file *file);
void remove_file_from_fdt(int fd);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	lock_init(&filesys_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	int syscall_num = f->R.rax;
	switch(syscall_num) {
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			exit(f->R.rdi);
			break;
		// case SYS_FORK:
			// fork(f->R.rdi);
		// 	break;
		// case SYS_EXEC:
		// 	exec(f->R.rdi);
		// 	break;
		// case SYS_WAIT:
			// wait(f->R.rdi);
		// 	break;
		case SYS_CREATE:
			f->R.rax = create(f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			f->R.rax = remove(f->R.rdi);
			break;
		case SYS_OPEN:
			f->R.rax = open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize(f->R.rdi);
			break;
		case SYS_READ:
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			seek(f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell(f->R.rdi);
			break;
		case SYS_CLOSE:
			close(f->R.rdi);
			break;
		default:
			exit(-1);
			thread_exit ();
			break;
	}
}

void halt() {
	power_off();
}

void exit(int status) {
	struct thread *cur = thread_current();
	cur->exit_status = status;
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

pid_t fork(const char *thread_name) {
	check_address(thread_name);

}

int exec(const char *cmd_line) {
	check_address(cmd_line);
	int file_size = strlen(cmd_line) + 1;
	char *fn_copy = palloc_get_page(2);
	if(fn_copy == NULL) {
		exit(-1);
	}
	strlcpy(fn_copy, cmd_line, file_size);
	if(process_exec(fn_copy) == -1) {
		return -1;
	}
	NOT_REACHED();
	return 0;
}

int wait(pid_t pid) {

}

bool create(const char *file, unsigned initial_size) {
	check_address(file);						// file 이름을 담은 주소가 유효한지 확인
	return filesys_create(file, initial_size);	// file.c의 함수를 이용하여 file에 담긴 이름과 initial_size의 크기를 가진 file 생성
}

bool remove(const char *file) {
	check_address(file);						// file 이름을 담은 주소가 유효한지 확인
	return filesys_remove(file);				// file.c의 함수를 이용하여 file 삭제
}

int open(const char *file) {
	check_address(file);		// file 주소가 유효한지 검사
	struct file *opened = filesys_open(file);	// file.c에 있는 file이라는 이름을 가진 파일을 open하는 함수
	if(opened == NULL) {
		return -1;
	}
	
	int fd = add_file_to_fdt(opened);	// 현재 프로세스의 파일 디스크립터 테이블에 추가하고 저장한 인덱스 가져옴

	if(fd == -1) {			// 제대로 저장되지 않았다면
		file_close(opened);	// 파일을 닫아줌
	}

	return fd;
}

int filesize(int fd) {
	struct file *file = find_file_by_fd(fd);	// fd를 통해 현재 프로세스의 fdt에서 file을 찾음
	if(file == NULL) {	// 안나오면 없다는 것이므로
		return -1;		// -1 return
	}
	return file_length(file);	// file.c에 있는 file의 길이를 구하는 함수로 file size 반환
}

int read(int fd, void *buffer, unsigned size) {
	check_address(buffer);
	off_t read_byte;
	uint8_t *read_buffer = buffer;		// char형 변수는 1byte이므로 uint8_t 형으로 buffer를 변환하여 선언
	if(fd == 0) {			// stdin
		char key;	// 입력 받은 문자
		for(read_byte = 0; read_byte < size; read_byte++) {
			key = input_getc();			// input.c에 있는 내장 함수로, input buffer에 있는 char를 가져옴
			*read_buffer++ = key;		// read_buffer를 증가시키며 key를 저장시킴(이때, 8씩 증가함)
			if(key == '\0') {			// NULL 문자가 들어온 경우 입력의 끝을 입력하므로 break
				break;
			}
		}
	}
	else if(fd == 1) {		// stdout
		return -1;
	}
	else {
		struct file* read_file = find_file_by_fd(fd);
		check_address(read_file);
		if(read_file == NULL) {
			return -1;
		}
		lock_acquire(&filesys_lock);			// 다른 thread에서 filesys를 사용하지 못하게 lock을 걸어놓고
		read_byte = file_read(read_file, buffer, size);	// file.c에 있는 내장 함수를 이용하여 read_file내에서 size만큼을 buffer에 복붙
		lock_release(&filesys_lock);			// filesys를 사용할 수 있도록 lock 해제
	}
	return read_byte;			// 얼마나 읽어왔는지 return
}

int write(int fd, const void *buffer, unsigned size) {
	check_address(buffer);			// buffer의 주소가 유효한지 확인
	if(fd == 0) {		// stdin(fd가 표준 입력에 대한 값이면 이 함수와 관련이 없으므로 return)
		return -1;
	}
	else if(fd == 1) {	// stdout(fd가 표준 출력에 관한 경우 화면에 출력)
		putbuf(buffer, size);		// 출력하는 함수
		return size;
	}
	else {				// 그 외에는 fd을 가진 파일에 buffer에서 size크기 만큼의 값을 쓰는 것임
		struct file *write_file = find_file_by_fd(fd);
		check_address(write_file);		// write_file 주소가 유효한지 검사
		if(write_file == NULL) {		// 검사 2
			return -1;
		}
		lock_acquire(&filesys_lock);			// 다른 thread에서 filesys를 사용하지 못하게 lock을 걸어놓고
		off_t write_result = file_write(write_file, buffer, size);	// file.c에 있는 내장 함수를 이용하여 write_file에 buffer에서 size크기만큼을 복붙
		lock_release(&filesys_lock);			// filesys를 사용할 수 있도록 lock 해제
		return write_result;		// 얼마나 썼는지 return
	}
}

void seek(int fd, unsigned position) {		// 파일 디스크립터 fd를 가진 파일에서 position으로 위치를 이동하는 함수
	if(fd <= 2) {
		return;
	}
	struct file *seek_file = find_file_by_fd(fd);
	check_address(seek_file);
	if(seek_file == NULL) {
		return;
	}
	file_seek(seek_file, position);
}

unsigned tell(int fd) {			// 파일 디스크립터 fd를 가진 파일의 현재 위치를 알려주는 함수
	struct file *tell_file = find_file_by_fd(fd);
	if(tell_file <= 2) {
		return;
	}
	return file_tell(tell_file);	// file.c의 함수 file_tell을 통해 파일의 위치 반환
}

void close(int fd) {
	struct file *to_close = find_file_by_fd(fd);	// 삭제할 파일이 파일 디스크립터 테이블에 존재하는지 확인
	if(to_close == NULL) {
		return;
	}
	remove_file_from_fdt(fd);		// 존재한다면 삭제
}

void check_address(void *addr) {
	struct thread *cur = thread_current();
	if(addr == NULL || is_kernel_vaddr(addr) ||	// 받아온 주소 값이 NULL이거나 kernel 영역을 가리키거나
		pml4_get_page(cur->pml4, addr) == NULL)	{// 할당된 페이지가 NULL이라면
		exit(-1);		// 해당 주소에 접근을 불가능하게 만듦
		}
}

static struct file *find_file_by_fd(int fd) {	// file descriptor 값이 주어지면
	struct thread *cur = thread_current();		
	if(fd < 0 || fd >= FD_LIMIT) {			// 유효한 fd 값이라면
		return NULL;
	}
	return cur->fd_table[fd];					// 현재 프로세스의 파일 디스크립터 테이블에서 찾음
}

// file을 현재 프로세스에서 열었을 때, 해당 프로세스의 파일 디스크립터 테이블에 추가하는 함수
int add_file_to_fdt(struct file *file) {
	struct thread *cur = thread_current();
	struct file **fdt = cur->fd_table;

	// file을 넣을 수 있는 자리를 찾음
	while(cur->fd_idx < FD_LIMIT && fdt[cur->fd_idx]) {
		cur->fd_idx++;
	}
	
	// 만약 index가 제한 범위를 넘어서면 저장하는 것이 불가능하므로 return -1;
	if(cur->fd_idx >= FD_LIMIT) {
		return -1;
	}
	
	// 안 넘으면 저장
	fdt[cur->fd_idx] = file;
	return cur->fd_idx;			// 저장한 인덱스(fd)를 return
}

void remove_file_from_fdt(int fd) {
	struct thread *cur = thread_current();

	if(fd < 0 || fd >= FD_LIMIT) {		// 제거하려는 fd가 유효하지 않다면 return
		return;
	}

	cur->fd_table[fd] = NULL;				// 현재 프로세스의 파일 디스크립터 테이블에서 fd에 해당하는 곳 NULL로 바꿈
}

