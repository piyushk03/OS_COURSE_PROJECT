#include "os2.h"

void init()
{
    memset(M, '\0', 1200);
    memset(IR, '\0', 4);
    memset(R, '\0', 4);
    C = 0;//toggle register 
    SI = 0;
    PI = 0;
    TI = 0;
    breakFlag = false;
}

void Terminate(int EM, int EM2=-1)
{
    fout << endl << endl;
    if(EM == 0){
        fout << " terminated normally. " << error[EM]<<endl;
    }
    else{
        fout <<" terminated abnormally due to " << error[EM] << (EM2 != -1 ? (". " + error[EM2]) : "") << endl;
        fout << "IC="<<IC<<", IR="<<IR<<", C="<<C<<", R="<<R<<", TTL="<<pcb.TTL<<", TTC="<<pcb.TTC<<", TLL="<<pcb.TLL<<", LLC="<<pcb.LLC;
    }
}
void read(int RA){
    fin.getline(buffer, 41);

    char temp[5];
    memset(temp, '\0', 5);
    memcpy(temp,buffer,4);

    if(!strcmp(temp, "$END"))//if there is no data card then 
    {
        Terminate(1);
        breakFlag = true;//program is terminated 
    }
    else{
        strcpy(M[RA],buffer);//copy all contents of buffer into the memory allocation RA 
    }
}

void write(int RA){
    if(pcb.LLC+1 > pcb.TLL)//It is checks that the line it is going to print will generate the line limit exceeded error or not
    {
        Terminate(2);
        breakFlag = true;
    }
    else
    {
        char str[40];
        int k = 0;
        for(int i=RA; i<(RA+10); i++)
        {
            for(int j=0; j<4; j++)
                str[k++] = M[i][j];//copy the contents of data from memory and store it in str 
        }
        fout << str << endl;//print in the output file 

        //printing the memory ka content
        for(int i=0; i<300; i++)
        {
            cout<<"M["<<i<<"] :";
            for(int j=0 ; j<4; j++)
            {
                cout<<M[i][j];
            }
            cout<<endl;
        }
        cout<<endl;
        pcb.LLC++;
    }
}

int mos(int RA = 0)
{
    if(TI == 0)
    {
        if(SI != 0)
        {
            switch(SI)
            {
                case 1:
                    read(RA);
                    break;
                case 2:
                    write(RA);
                    break;
                case 3:
                    Terminate(0);
                    breakFlag = true;
                    break;
                default:
                    cout<<"Error with SI."<<endl;
            }
            SI = 0;//again initialize the SI with 0 to avoid mess in further steps
        }
        else if(PI != 0)
        {
            switch(PI){
                case 1:
                    Terminate(4);
                    breakFlag = true;
                    break;
                case 2:
                    Terminate(5);// when TI=0 AND PI=2 - it gives error message 5 operand error 
                    breakFlag = true;
                    break;
                case 3:
                    PI = 0;
                    //Page Fault checking
                    char temp[3];
                    memset(temp,'\0',3);
                    memcpy(temp, IR, 2);

                    if(!strcmp(temp,"GD") || !strcmp(temp,"SR"))
                    {
                        int m;
                        do
                        {
                            m = allocate();//allocate and generate free random number 
                        }while(M[m*10][0]!='\0');

                        int currPTR = PTR;
                        while(M[currPTR][0]!='*') 
                            currPTR++;

                        itoa(m, M[currPTR], 10);//put that frame number in that page table - updating the page table 

                        cout << "Valid Page Fault, page frame = " << m << endl;
                        cout << "PTR = " << PTR << endl;
                        //print the memory containing the page table where the program card page and page where data is stored is stored
                        for(int i=0; i<300; i++)
                        {
                            cout<<"M["<<i<<"] :";
                            for(int j=0 ; j<4; j++)
                            {
                                cout<<M[i][j];
                            }
                            cout<<endl;
                        }
                        cout<<endl;
                        
                        if(pcb.TTC+1 > pcb.TTL)
                        {
                            TI = 2;
                            PI = 3;
                            mos();
                            break;
                        }
                        pcb.TTC++;//increment TTC by one 
                        return 1;
                    }
                    else if(!strcmp(temp,"PD") || !strcmp(temp,"LR") || !strcmp(temp,"H") || !strcmp(temp,"CR") || !strcmp(temp,"BT"))
                    {
                        Terminate(6);
                        breakFlag = true;

                        if(pcb.TTC+1 > pcb.TTL)//check for all other instruction if TTC is greater than TTL then time limit exceeded    
                        {
                            TI = 2;
                            PI = 3;
                            mos();
                            break;
                        }
                        //pcb.TTC++;
                    }
                    else
                    {
                        PI = 1;//opcode error if it is HD10
                        mos();
                    }
                    return 0;
                default:
                    cout<<"Error with PI."<<endl;
            }
            PI = 0;
        }
    }
    else{
        if(SI != 0)
        {
            switch(SI)
            {
                case 1:
                    Terminate(3);
                    breakFlag = true;
                    break;
                case 2:
                    write(RA);//we have incremented the TTC before coming to master mode so we need to write before termination to complete the operation by assuming that this instruction is executed 
                    if(!breakFlag) Terminate(3);
                    break;
                case 3:
                    Terminate(0);
                    breakFlag = true;
                    break;
                default:
                    cout<<"Error with SI."<<endl;
            }
            SI = 0;
        }
        else if(PI != 0)
        {
            switch(PI)
            {
                case 1:
                    Terminate(3,4);
                    breakFlag = true;
                    break;
                case 2:
                    Terminate(3,5);
                    breakFlag = true;
                    break;
                case 3:
                    Terminate(3);
                    breakFlag = true;
                    break;
                default:
                    cout<<"Error with PI."<<endl;
            }
            PI = 0;
        }
    }

    return 0;
}

int addressMap(int VA)
{
    if(0 <= VA && VA < 100)
    {
        int pte = PTR + VA/10;//memory location where the frame number of that page is stored 
        //if there is no frame for the given pte then there is page fault 
        if(M[pte][0] == '*')
        {
            PI = 3;
            return 0;
        }
        cout << "In addressMap(), VA = " << VA << ", pte = " << pte << ", M[pte] = " << M[pte] << endl;
        return atoi(M[pte])*10 + VA%10;//return the memory location of the instruction of that page 
    }
    else
    {
        PI = 2;//since VA is not from 0 to 99 this gives operand error nd call for master mode 
    }
    return 0;
}

void execute_user_program()
{
    char temp[3], loca[2];
    int locIR, RA; //These variables will be used to hold various data throughout the execution of the program

    while(true) //This is a while loop that will continue running until the breakFlag variable is set to true.
    {
        if(breakFlag) 
            break;

        RA = addressMap(IC);//to get the physical   memory location instruction is located 
        //This is a conditional statement that checks if the PI (Program Interrupt) flag has been set to a non-zero value. If it has, the mos() function is called to handle the interrupt. If mos() returns true, the loop continues; otherwise, the loop is broken and the function exits.
        if(PI != 0)
        {
            if(mos())
            {
                continue;
            }
            break;
        }
        cout << "IC = " << IC << ", RA = " << RA << endl;
        memcpy(IR, M[RA], 4);           //Memory to IR, instruction fetched
        IC += 1;


        memset(temp,'\0',3);
        memcpy(temp,IR,2);// to copy the first two bytes of the current instruction in IR into the temp array.
        //that checks the next two bytes of the current instruction in IR to make sure they represent a valid memory location.
        for(int i=0; i<2; i++)
        {
            //The condition checks if the character is a digit (ASCII values between 48 and 57) or a null character (ASCII value 0).
            //If the character is not a digit or null, it means the operand location is invalid, and the condition evaluates to true.
            if(!((47 < IR[i+2] && IR[i+2] < 58) || IR[i+2] == 0)){
                PI = 2;
                break;
            }
            loca[i] = IR[i+2];//If they do, the location is stored in the loca array. 
        }
        //If not, the PI flag is set to 2 -operand error 
        if(PI != 0)
        {
            mos();
            break;
        }

        //loca[0] = IR[2];
        //loca[1] = IR[3];
        locIR = atoi(loca);//These lines convert the loca array (which contains the memory location of the operand for the current instruction) to an integer (locIR) using the atoi() function. 

        RA = addressMap(locIR);
        //for pointing out to the same GD instruction again we decrement the IC by 1

        //At 2nd time RA contains the address of the memory location where data is being stored 
        //for valid page fault 
        if(PI != 0)
        {
            if(mos())//if error is handled by the mos   
            {
                IC--;
                continue;
            }
            break;//means that the error could not be handled and the program should be terminated. 
        }

        cout << "IC = " << IC << ", RA = " << RA << ", IR = " << IR << endl;
        if(pcb.TTC+1 > pcb.TTL)
        {
            TI = 2;
            PI = 3;
            mos();
            break;
        }

        if(!strcmp(temp,"LR"))
        {
            memcpy(R,M[RA],4);//just copy the instruction from memory to general purpose register 
            pcb.TTC++;
        }
        else if(!strcmp(temp,"SR"))
        {
            memcpy(M[RA],R,4);//copy the instruction from register to the memory
            pcb.TTC++;
        }
        else if(!strcmp(temp,"CR"))
        {
            if(!strcmp(R,M[RA]))//compare the contents of register R and memory location if they are same then set toggle register to true
                C = 1;
            else
                C = 0;
            pcb.TTC++;
        }
        else if(!strcmp(temp,"BT"))
        {
            if(C == 1)//if toggle register is true and copy the current operand instruction
                IC = RA;
            pcb.TTC++;
        }
        else if(!strcmp(temp,"GD"))
        {
            SI = 1;
            mos(RA);
            pcb.TTC++;
        }
        else if(!strcmp(temp,"PD"))
        {
            SI = 2;
            mos(RA);
            pcb.TTC++;
        }
        else if(!strcmp(temp,"H"))
        {
            SI = 3;
            mos();
            pcb.TTC++;
            break;
        }
        else // temp contains instruction other than GT,PD,LR,SR,CR then there is opcode error
        {
            PI = 1;
            mos();
            break;
        }
        memset(IR, '\0', 4);//clears the contents of register 
    }
}

void start_execution()
{
    IC = 0;
    execute_user_program();
}

int allocate()
{
    srand(time(0));//to seed the random number generator (RNG) with a unique value based on the current time The RNG uses this seed value to generate a sequence of seemingly random numbers By passing the current time as the argument to srand(), we ensure that a different seed value is used every time the code is executed
    return (rand() % 30);//generate a random number from 0 to 29
}

void load()
{
    int m;                                  //Variable to hold memory location
    int currPTR;                            //Points to the last empty location in Page Table Register
    char temp[5];                           //Temporary Variable to check for $AMJ, $DTA, $END
    memset(buffer, '\0', 40);              //clears the buffer  

    while(!fin.eof())
    {
        fin.getline(buffer,40);
        memset(temp, '\0', 5); //Clear the temporary array to all null characters
        memcpy(temp,buffer,4);//Clears the temporary array and copies the first 4 characters of the buffer into temp to check for certain control strings.

        if(!strcmp(temp,"$AMJ"))
        {
            init();

            int jobId, TTL, TLL;
            memcpy(temp, buffer+4, 4);//buffer is also a pointer variable representing the source address of the data. buffer+4 is pointer arithmetic that adds an offset of 4 to the buffer address. This means that the copying will start from the fourth byte (index 4) of the buffer.
            jobId = atoi(temp);//Convert the job ID from a string to an integer
            memcpy(temp, buffer+8, 4);
            TTL = atoi(temp);
            memcpy(temp, buffer+12, 4);
            TLL = atoi(temp);
            pcb.setPCB(jobId, TTL, TLL);//set the jobId ,TTL,TTL pcb object

            //Generating a page table for choosing random number between 0 to 29 aas main memory as 30 pages 
            PTR = allocate()*10;//gives starting address of page table
            memset(M[PTR], '*', 40);//set all the bytes '*' of that page table 
            currPTR = PTR;//staring address stored in currPTR
        }
        //Again read next line from input file 
        else if(!strcmp(temp,"$DTA"))
        {
            start_execution();
        }
        else if(!strcmp(temp,"$END"))
        {
            continue;
        }
        else //if it is program card 
        {
            if(breakFlag) // the function checks if the break flag is set. //Skip to the next line in the input file
                continue;

            do  //Loop until a free frame is found
            {
                m = allocate();//again generate free frame number to load the program card  m stores the frame number where I will load the program(instruction)
            }while(M[m*10][0]!='\0');//If the frame is not empty, continue looping

            itoa(m, M[currPTR], 10);//Convert the frame number to a string and store it in the page table - updating the page table and The third argument to itoa() is the base of the conversion. In this case, it's set to 10, indicating that the integer value m will be converted to a decimal string representation.
            currPTR++;

            strcpy(M[m*10],buffer);//load 1st program into the page whose starting address is m 

            cout << "PTR = " << PTR << endl;
            //printing the memory ka content
            for(int i=0; i<300; i++)
            {
                cout<<"M["<<i<<"] :";
                for(int j=0 ; j<4; j++)
                {
                    cout<<M[i][j];
                }
                cout<<endl;
            }
            cout<<endl;
        }
    }
}

int main(){
    load();
    fin.close(); 
    fout.close();
    return 0;
}
