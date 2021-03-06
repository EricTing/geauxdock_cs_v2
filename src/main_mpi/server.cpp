#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <size.h>

#include <mpi.h>

using namespace std;

MPI_Datatype MPI_MSG;




class Mytask
{
public:
    int machine_id;
    int machine_type;
    float perf; // performance
    float power; // power consuption

    Mytask(int id) {
        machine_id = id;
        machine_type = 0;
        perf = 0;
        power = 0;
    }
};




// assuming the MPI rank of server is 0
// maybe this restriction can be removed !!!!!!!!!!!!!!!!!!!!!
void distribute(vector<string> & lines)
{
    int id, nprocs;
    MPI_Comm_rank (MPI_COMM_WORLD, &id);
    MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
    MPI_Request req[nprocs];
    MPI_Status st[nprocs];
    MPI_Status status;


    int task_n = lines.size() - 1; // number of input data;
    int task_counter = 0;
    int worker_counter_quit = 0;
    deque<Mytask> myqueue;
    deque<int> unsync_msg_queue;


    while (1) {
        // receiving request
        int dst;
        const int mytag = 0;
        MPI_Recv (&dst, 1, MPI_INT, MPI_ANY_SOURCE, mytag, MPI_COMM_WORLD, &status);
        Mytask t(dst);
        myqueue.push_back(t);


        // serving clients on the queue
        while (myqueue.size() != 0) {
            auto i = myqueue.begin() - 1;
            int dst = i->machine_id;

            const char msg[MAXINPUTSTRINGLENG];
            if (task_counter != task_n) {
                strncpy(msg, lines[taski + 1].c_str());
                task_counter++;
            }
            else {
                strncpy(msg, "FINISH_SIGNAL");
                worker_counter_quit++;
            }


            const int mytag = 0;
            MPI_Isend (msg, 1, MPI_MSG, dst, mytag, MPI_COMM_WORLD, &req[dst]);
            unsync_msg_queue.push_back(dst);

            myqueue.erase(i);
            printf ("server sent msg %03d to client %02d\n", task_counter, dst);
        }

        // wait for MPI messages
        while (unsync_msg_queue.size() != 0) {
            auto i = unsync_msg_queue.begin() - 1;
            MPI_Wait (&req[*i], &st[*i]);
            unsync_msg_queue.erase(i);
        }



        // quit
        if (worker_counter_quit >= nprocs - 1)
            break;
    }

}






void load(const string fnpath)
{
    ifstream ifn(fnpath.c_str());
    if (!ifn.is_open()) {
        cout << "Failed to open " << fnpath << endl;
        exit(EXIT_FAILURE);
    }

    string line;
    vector<string> lines;

    while (getline(ifn, line)) {
        //cout << line << endl;
        lines.push_back(line);
    }

    ifn.close();

    const int nlines = lines.size();  // number of lines
    if (nlines < 2) {
        cout << "number of lines is wrong" << endl;
        exit(EXIT_FAILURE);
    }


    distribute(lines);
}


void server(int argc, char **argv)
{
    if (argc == 1) {
        load("a2.csv");
    }
    else if (argc == 2) {
        load(argv[1]);
    }
    else {
        printf("%s <FILE(*.csv)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}


int
main (int argc, char **argv)
{
    int id, nprocs;
    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &id);
    MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
    if (nprocs < 2) {
        printf ("nprocs < 2. exit\n");
        exit (-1);
    }


    MPI_Type_contiguous (sizeof(char) * MAXINPUTSTRINGLENG, MPI_BYTE, &MPI_MSG);
    MPI_Type_commit (&MPI_MSG);


    if (id == 0)
        server (argc, argv); // server ID must be 0; otherwise the messsage ID will be wrong
    else
        printf ("server is assigned with a wrong MPI rank\n");


    return 0;
}




