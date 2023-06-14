#include <iostream>
#include <fstream>
#include <string.h>
#include <time.h>
using namespace std;

/*File Handlers*/
ifstream fin("input.txt");
ofstream fout("output.txt");

/*Memory*/
char M[300][4];
char buffer[40];
char IR[4];
char R[4];
int IC;
int C;
int SI; 
int PI;
int TI;
int PTR;//Starting address og page table 
bool breakFlag;//It indicates job is finished or not

/*Process Control Block*/
struct PCB
{
    int job_id;
    int TTL;
    int TLL;
    int TTC;//increment after execution of each instruction
    int LLC;//when PD comes it will increment count by 1

    void setPCB(int id, int ttl, int tll)
    {
        job_id = id;
        TTL = ttl;
        TLL = tll;
        TTC = 0;
        LLC = 0;
    }
};

PCB pcb;

/*Error Messages*/
string error[7] = {"No Error", "Out of Data", "Line Limit Exceeded", "Time Limit Exceeded",
    "Operation Code Error", "Operand Error", "Invalid Page Fault"};

/*Functions*/
void init();
void read(int RA);
void write(int RA);
int addressMap(int VA);
void execute_user_program();
void start_execution();
int allocate();
void load();
