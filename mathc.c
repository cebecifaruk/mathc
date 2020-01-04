#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

unsigned int mode;

FILE *fdest,*fsource;

char *help="\nUsage:mathc [option] [filename]\nOptions\n\t-i\tinterpreter mode\n\t-c\tcompiler mode\n";
char *startOfAssembly="\/\/Generated from mathc	\n\t .data\n.print_format: .string \"\\n>%d\"\n\n\t.text \n\t.globl main \n\t.type main,@function\nmain:\n\tpushq %rbp\n\tmovq %rsp, %rbp\n\n\n";
char *endOfAssembly="\n\tret\n";
char *endOfAssemblyLine="\n\tpop %rsi\n\tmovl %esi,ans(%eip)\n\tmovl $.print_format, %edi\n\tmovl $0, %eax\n\tcall	printf\n\t \/\/End of Line";


// Variable List (Lnked List)

struct varNode {
	struct varNode *next;
	int data;
	char name[0];
}root={.name="ans",.data=0,.next=NULL};

struct varNode *iter=&root;

int parser (char *, unsigned int);
int read_var (char*,unsigned int);
int write_var(char*,unsigned int,int);

int main (int argc, char *argv[]) {
	unsigned int i;
	char *line;
	fdest=stdout;
	fsource=stdin;

	for (i=1;i<argc;i++) if(argv[1][0] == '-') if(!argv[i][2]) switch(argv[i][1]) {

		case 'i': i++; if(!(fsource=fopen(argv[i],"r"))) exit(-1);
		case 's': mode=1;
			for(	printf("\n>");
				fgets(line,200,fsource);
				printf("ans=%d\n>",write_var("ans",3,parser(line,strlen(line)))));
			break;

		case 'c': mode=0; i++; if(!(fsource=fopen(argv[i],"r"))) exit(-1);
			for(	fprintf(fdest,"%s",startOfAssembly);
				fgets(line,200,fsource);
				parser(line,strlen(line)),fprintf(fdest,"%s\n\n",endOfAssemblyLine));
			  fprintf(fdest,"\n\t%s",endOfAssembly);
				list_var_to_data_segment();
			  break;


		case 'h':
		default: printf("%s",help);break;
	}
	fclose(fdest);
	fclose(fsource);
	return 0;
}

int atoin (char *str, unsigned size)
{
	char *dest=malloc(size+1);
	strncpy(dest,str,size);
	int buf = atoi(dest);
	free (dest);
	return buf;
}



int read_var(char *name, unsigned int len)
{
	struct varNode *node = &root;
	while (strncmp(node->name,name,len)!=0 && node->next) node=node->next;
	if (strncmp(node->name,name,len)==0) return node->data;
	else printf("ERROR");
}

int write_var(char *name, unsigned int len, int data)
{
	struct varNode *node=&root;
	while (strncmp(node->name,name,len)!=0 && node->next) node=node->next;
	if (strncmp(node->name,name,len)==0) {node->data=data;return data;}
	iter->next = malloc(sizeof(struct varNode)+len);
	iter->next->next=NULL;
	iter->next->data=data;
	strncpy (iter->next->name,name,len);
	iter=iter->next;
	return data;
}

void list_var_to_data_segment ()
{
	fprintf(fdest,"\n\n\n\n	.data\n");
	struct varNode *node=&root;
	for(;node->next;node=node->next) fprintf(fdest,"\n%s: .long 0",node->name);
	fprintf(fdest,"\n%s: .long 0\n",node->name);
}

int parser (char *exp, unsigned int len)
{
	int i,j=0, a=0, b=0, numOfBracket=0,thisBracketNumber=0, bra=0;
	unsigned int startOfBracket=0, stopOfBracket=len-1, op=0;

	//Step 1 - Operation & Bracket Analysis
	for (i=len-1;i>=0 ;i--) {
		switch (exp[i]) {

			case '(':
				if(bra == thisBracketNumber) startOfBracket=i+1;
				thisBracketNumber--;
				break;
			case ')':
				thisBracketNumber++;numOfBracket++;
				if(bra == thisBracketNumber) stopOfBracket=i-1;
				break;


			case '*': if(op<1 && bra == thisBracketNumber){op=1;j=i;}; break;
			case '/': if(op<1 && bra == thisBracketNumber){op=2;j=i;}; break;
			case '+': if(op<3 && bra == thisBracketNumber){op=3;j=i;}; break;
			case '-': if(op<3 && bra == thisBracketNumber){op=4;j=i;}; break;
			case '=': if(op<5 && bra == thisBracketNumber){op=5;j=i;}; break;
			default: break;

		}
		if(i==0 && op==0 && bra< numOfBracket) {bra++;i=len;numOfBracket=0;thisBracketNumber=0;}
		//TODO: bracket error
	}

	//Step 2 - Recursive
	if(op && op != 5)
	{
		a=parser(exp+startOfBracket,j-startOfBracket);
		b=parser(exp+j+1,stopOfBracket-j);
	}

	//Step 3 - Operation and Compile

	char *ptr;
	if(mode!=0) switch (op) {
		case 0:
			for(ptr=exp+startOfBracket,i=0;!isalpha(*(ptr+i)) && i<len;i++);
			if(!(i<len)) return atoin(ptr,len);
			for(;isblank(*ptr);ptr++,i++);
			for(i=0;isalpha(*(ptr+i))&&(ptr+i)!=(exp+startOfBracket+len);i++);
			return read_var(ptr,i);
		case 5:
			for(ptr=exp+startOfBracket;isalpha(*ptr)==0;ptr++);
			for(i=0;isalpha(*(ptr+i));i++);
			int data = parser(exp+j+1,stopOfBracket-j);
			write_var(ptr,i,data);
			return data;

		case 1: return a*b;
		case 2: return a/b;
		case 3: return a+b;
		case 4:	return a-b;
	}

	else switch(op) {
		case 0: 
				for(ptr=exp+startOfBracket,i=0;!isalpha(*(ptr+i)) && i<len;i++);
				if(!(i<len)) {fprintf(fdest,"\n	pushq $%.*s",len,exp+startOfBracket); return 0;}
				for(;isblank(*ptr);ptr++,i++);
				for(i=0;isalpha(*(ptr+i))&&(ptr+i)!=(exp+startOfBracket+len);i++);
				fprintf(fdest,"\n	mov	$0,%%rax\n	movl %.*s(%%eip),%%eax\n	push %rax",i,ptr);return 0;

		case 1: fprintf(fdest,"\n	pop %%rcx\n	pop %%rax\n	mul %%ecx\n		push %%rax");return 0;
		case 2: fprintf(fdest,"\n	pop %%rcx\n	pop %%rax\n	mov $0,%%edx\n	div %%ecx\n		push %%rax");return 0;
		case 3: fprintf(fdest,"\n	pop %%rcx\n	pop %%rax\n	add %%ecx,%%eax\n	push %%rax");return 0;
		case 4: fprintf(fdest,"\n	pop %%rcx\n	pop %%rax\n	sub %%ecx,%%eax\n	push %%rax");return 0;
		
		case 5: for(ptr=exp+startOfBracket;isalpha(*ptr)==0;ptr++);
			for(i=0;isalpha(*(ptr+i));i++);
			parser(exp+j+1,stopOfBracket-j);
			fprintf(fdest,"\n	pop %%rcx\n	movl %%ecx,%.*s(%%rip)\n	push %%rcx",i,ptr);
			write_var(ptr,i,0);		
			return 0;
	}
}
