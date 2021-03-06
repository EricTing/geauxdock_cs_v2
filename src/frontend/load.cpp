/*
   ==============================================================================================
   _________________ ___ __              .__             _      ________   ____
   /  ____/\_____   \    |   \           _| /___   ___ |  | _ /   ____/  /     \
   w
   /   \  __ |     __/    |   /  _____  / _ |/   \/ __\|  |/ / \____  \  /  \ /  \
   \    \\  \|    |   |    |  /  /____/ / // (  <> )  \__|    <  /        \/    Y    \
   \_____  /|___|   |_____/           \___ |\___/ \__  >_| \/______  /\___|_  /
   \/                                  \/           \/     \/        \/         \/

   GPU-accelerated hybrid-resolution ligand docking using ReplicaMC Exchange Monte Carlo

   ==============================================================================================
   */


#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>		// for max
#include <sstream>
#include <cmath>
#include <cstdlib>		// for EIXT_SUCCESS
#include <list>
#include <cstdio>
#include <cstring>
#include <vector>

#include <geauxdock.h>
#include <data.h>
#include <rmsd.h>
#include <size.h>
#include <load.h>

//#include <yeah/cpp/timer.hpp>


using namespace std;



void
pushLigandPoint (Ligand0 * lig, int n, string a, int t, float c)
{
    lig->n[n] = n;
    lig->a[n] = a;
    lig->t[n] = t;
    lig->c[n] = c;
}




/*
 * read 2D matrix
 */
vector < vector < float > >
read2D(TraceFile *trace_file)
{
    string fn = trace_file->path;
    vector < vector < float > > matrix;
    string line;
    float val;

    ifstream file(fn.c_str());

    if (file.is_open()) {
        cout << "reading near native traces from " << fn << endl;
    }
    else {
        cout << "Error opening file " << fn << endl;
    }

    while(getline(file, line)) {
        if (isdigit(line[0])) {
            vector < float > row;
            istringstream iss(line);
            while (iss >> val) {
                row.push_back(val);
            }
            matrix.push_back(row);
        }
    }

    return matrix;
}


void
loadLigand( LigandFile * lig_file, Ligand0 * lig)
{
    const std::string molid = lig_file->molid;
    std::vector<string> lines;
    int nlines;
    string line;
    int lna, lnb;

    ifstream ifn(lig_file->path.c_str());
    if (!ifn.is_open()) {
        cout << "Failed to open " << lig_file->path << endl;
        exit(EXIT_FAILURE);
    }
    while (getline(ifn, line)) {
        lines.push_back(line);
        if (line == "$$$$")
            break;
    }
    ifn.close();

    nlines = lines.size();
    if (nlines <= 10) {
        printf("Ligand file too small, quit\n");
        exit(EXIT_FAILURE);
    }

    lna = atoi(lines[3].substr(0, 3).c_str());
    lnb = atoi(lines[3].substr(3, 3).c_str());
    lig_file->lig_natom = lna;
    lig_file->lnb = lnb;

    for (int i = 4 + lna + lnb; i < nlines; i++) {
        if (lines[i].find(molid) != string::npos) // modlid id
            lig_file->id = lines[i + 1];
        if (lines[i].find("ENSEMBLE_TOTAL") != string::npos)
            lig_file->raw_conf = atoi(lines[i + 1].c_str());
    }





    // load the default conformation and write the ensemble coords to tmp matrix
    Ligand0 *mylig = &lig[0];
    mylig->lig_natom = lna;
    mylig->lnb = lnb;

    float tmp1[MAX_CONF_LIG][MAXLIG][3];	// for ensemble coords
    float tmp2[MAXLIG];	// for OB_ATOMIC_CHARGES
    int atom_types[MAXLIG];	// for atom types
    int tmp7 = 0;		// number the raw conf


    for (int i = 4 + lna + lnb; i < nlines; i++) {
        if (lines[i].find(molid) != string::npos)
            mylig->id = lines[i + 1];

        else if (lines[i].find("SMILES_CANONICAL") != string::npos)
            mylig->smiles = lines[i + 1];

        else if (lines[i].find("OB_MW") != string::npos)
            mylig->mw = atof(lines[i + 1].c_str());

        else if (lines[i].find("OB_logP") != string::npos)
            mylig->logp = atof(lines[i + 1].c_str());

        else if (lines[i].find("OB_PSA") != string::npos)
            mylig->psa = atof(lines[i + 1].c_str());

        else if (lines[i].find("OB_MR") != string::npos)
            mylig->mr = atof(lines[i + 1].c_str());

        else if (lines[i].find("MCT_HBD") != string::npos)
            mylig->hbd = atoi(lines[i + 1].c_str());

        else if (lines[i].find("MCT_HBA") != string::npos)
            mylig->hba = atoi(lines[i + 1].c_str());

        else if (lines[i].find("OB_ATOM_TYPES") != string::npos) {
            int tmp4 = 0;
            istringstream tmp5(lines[i + 1]);
            while (tmp5) {
                std::string tmp6;
                tmp5 >> tmp6;
                if (tmp6.length() > 0)
                    atom_types[tmp4++] = getLigCode(tmp6);
            }
        }

        else if (lines[i].find("OB_ATOMIC_CHARGES") != string::npos) {
            int tmp4 = 0;
            istringstream tmp5(lines[i + 1]);
            while (tmp5) {
                std::string tmp6;
                tmp5 >> tmp6;
                if (tmp6.length() > 0)
                    tmp2[tmp4++] = atof(tmp6.c_str());
                // DEBUG_2_("",atof(tmp6.c_str()));
            }
        }

        else if (lines[i].find("ENSEMBLE_COORDS") != string::npos)
            while (lines[++i].size() && tmp7 < MAX_CONF_LIG) {
                int tmp8 = 0;
                int tmp9 = 0;
                istringstream tmp5(lines[i]);
                while (tmp5) {
                    std::string tmp6;
                    tmp5 >> tmp6;
                    if (tmp6.length() > 0) {
                        tmp1[tmp7][tmp8][tmp9] = atof(tmp6.c_str());
                        if (++tmp9 > 2) {
                            tmp8++;
                            tmp9 = 0;
                        }
                    }
                }
                tmp7++;
            }
    }

    float tmp8[MAXLIG][3];

    /* write the properties of each atom of the default conf */
    for (int i1 = 0; i1 < mylig->lig_natom; i1++) {
        // DEBUG_1_("substr ", lines[i1].substr(31, 24).c_str());
        pushLigandPoint(mylig, i1, lines[i1 + 4].substr(31, 24).c_str(), atom_types[i1], tmp2[i1]);
        /* tmp8 contains coords xyz of the first conf */
        // /*
        tmp8[i1][0] = atof(lines[i1 + 4].substr(0, 10).c_str());
        tmp8[i1][1] = atof(lines[i1 + 4].substr(10, 10).c_str());
        tmp8[i1][2] = atof(lines[i1 + 4].substr(20, 10).c_str());
        // */

        // /*
        for (int i5 = 0; i5 < 3; i5++) {
            mylig->coord_orig.center[i5] += tmp8[i1][i5];
            mylig->pocket_center[i5] += tmp8[i1][i5];
        }
        // */

    }

    /* calculate the ligand center
     * pocket center overlaps with ligand center initially */
    for (int i5 = 0; i5 < 3; i5++) {
        mylig->coord_orig.center[i5] /= (float)mylig->lig_natom;
        mylig->pocket_center[i5] /= (float)mylig->lig_natom;
        // cout << "0--ligand center " << mylig->coord_orig.center[i5] << endl;
        // cout << "0--pocket center " <<  mylig->pocket_center[i5] << endl;
    }

    // calculate the relative coord
    for (int i1 = 0; i1 < mylig->lig_natom; i1++)
        for (int i5 = 0; i5 < 3; i5++)
            tmp8[i1][i5] -= mylig->coord_orig.center[i5];

    // push the relative coords
    int i4 = 0;
    for (i4 = 0; i4 < mylig->lig_natom; i4++) {
        mylig->coord_orig.x[i4] = tmp8[mylig->n[i4]][0];
        mylig->coord_orig.y[i4] = tmp8[mylig->n[i4]][1];
        mylig->coord_orig.z[i4] = tmp8[mylig->n[i4]][2];

        // cout << "0--coord " <<  mylig->coord_orig.x[i4] << endl;
        // cout << "0--coord " <<  mylig->coord_orig.y[i4] << endl;
        // cout << "0--coord " <<  mylig->coord_orig.z[i4] << endl;
    }

    // DEBUG_1_("raw_conf: ", lig_file->raw_conf);
    // copy the shared properties to its rest ensemble

    for (int i = 0; i < tmp7; i++) {
        mylig = &lig[(i+1)];
        mylig->lig_natom = lig_file->lig_natom;
        mylig->lnb = lig_file->lnb;
        for (int i1 = 4 + mylig->lig_natom + mylig->lnb; i1 < nlines; i1++) {
            if (lines[i1].find(molid) != string::npos) {
                mylig->id = lines[i1 + 1];
            }

            else if (lines[i1].find("SMILES_CANONICAL") != string::npos)
                mylig->smiles = lines[i1 + 1];

            else if (lines[i1].find("OB_MW") != string::npos)
                mylig->mw = atof(lines[i1 + 1].c_str());

            else if (lines[i1].find("OB_logP") != string::npos)
                mylig->logp = atof(lines[i1 + 1].c_str());

            else if (lines[i1].find("OB_PSA") != string::npos)
                mylig->psa = atof(lines[i1 + 1].c_str());

            else if (lines[i1].find("OB_MR") != string::npos)
                mylig->mr = atof(lines[i1 + 1].c_str());

            else if (lines[i1].find("MCT_HBD") != string::npos)
                mylig->hbd = atoi(lines[i1 + 1].c_str());

            else if (lines[i1].find("MCT_HBA") != string::npos)
                mylig->hba = atoi(lines[i1 + 1].c_str());

        }

        // copy the coord_center and the pocket center of the default to the rest
        for (int i5 = 0; i5 < 3; i5++) {
            mylig->coord_orig.center[i5] = lig[0].coord_orig.center[i5];
            mylig->pocket_center[i5] = lig[0].pocket_center[i5];
            // cout << "1--ligand center " << mylig->coord_orig.center[i5] << endl;
            // cout << "1--pocket center " <<  mylig->pocket_center[i5] << endl;
        }

        // copy the atom types and charges
        for (int i1 = 0; i1 < mylig->lig_natom; i1++) {
            pushLigandPoint(mylig, i1, lines[i1 + 4].substr(31, 24).c_str(), atom_types[i1], tmp2[i1]);
            /*
               DEBUG_1_("i1", i1);
               DEBUG_1_("atom_types ", atom_types[i1]);
               DEBUG_1_("tmp2 ", tmp2[i1]);
               */
        }
    }

    // exclude those confs with rmsd less than 0.1
    mylig = &lig[0];
    mylig->lens_rmsd = 0.0;
    int lig_conf_rest = 1;
    int *conf_ip;
    conf_ip = &lig_conf_rest;

    for (int i6 = 0; i6 < tmp7; i6++) {
        float weights[MAXLIG];
        float ref_xyz[MAXLIG][3];
        float mob_xyz[MAXLIG][3];

        for (int i7 = 0; i7 < mylig->lig_natom; i7++) {

            // cout << "3-- mylig->lig_natom " << mylig->lig_natom << endl;
            weights[i7] = 1.0;

            for (int i5 = 0; i5 < 3; i5++) {
                ref_xyz[i7][i5] = tmp8[i7][i5];  // native pose as the ref

                mob_xyz[i7][i5] = tmp1[i6][i7][i5];  // containing ensemble coords
            }
        }

        float mob_cen[3];

        for (int i5 = 0; i5 < 3; i5++)
            mob_cen[i5] = 0.0;

        for (int i7 = 0; i7 < mylig->lig_natom; i7++)
            for (int i5 = 0; i5 < 3; i5++)
                mob_cen[i5] += mob_xyz[i7][i5];

        for (int i5 = 0; i5 < 3; i5++)
            mob_cen[i5] /= (float)mylig->lig_natom;

        for (int i7 = 0; i7 < mylig->lig_natom; i7++)
            for (int i5 = 0; i5 < 3; i5++)
                mob_xyz[i7][i5] -= mob_cen[i5];

        int mode = 1;
        float rms1 = 0.0;
        float u[3][3];
        float t[3];
        int ier = 0;

        u3b_(&weights, &mob_xyz, &ref_xyz, &(mylig->lig_natom), &mode, &rms1, &u, &t, &ier);

        // cout << "0-- rms1 " << rms1 << endl;
        // cout << "0-- mylig.lig_natom " << mylig->lig_natom << endl;

        float rms2 = sqrt(rms1 / (float)mylig->lig_natom);

        if (rms2 > 0.1) {
            Ligand0 *my_rest_lig = &lig[*conf_ip];	// my_rest_lig points to other conformations in the data file
            // cout << "0-- lig_conf_rest  " << *conf_ip << endl;
            for (i4 = 0; i4 < mylig->lig_natom; i4++) {

                // cout << "1-- lig_conf_rest" << lig_conf_rest << endl;

                my_rest_lig->coord_orig.x[i4] = mob_xyz[mylig->n[i4]][0];
                my_rest_lig->coord_orig.y[i4] = mob_xyz[mylig->n[i4]][1];
                my_rest_lig->coord_orig.z[i4] = mob_xyz[mylig->n[i4]][2];

                /*
                   cout << "3-- conf " << * conf_ip  << endl;
                   cout << "3-- x " << my_rest_lig->coord_orig.x[i4] << endl;
                   cout << "3-- y " << my_rest_lig->coord_orig.y[i4] << endl;
                   cout << "3-- z " << my_rest_lig->coord_orig.z[i4] << endl;
                   */
            }

            my_rest_lig->lens_rmsd = rms2;
            *conf_ip += 1;

            // cout << "2-- lig_conf_rest " << *conf_ip << endl;
        }

        if (*conf_ip >= MAX_CONF_LIG)
            break;

    }
    lig_file->conf_total = *conf_ip;
}




void
pushProteinPoint (Protein0 * prt, int r, int n, int t, int d, int c)
{
    int i = prt->prt_npoint;

    prt->r[i] = r;
    prt->n[i] = n;
    prt->t[i] = t;
    prt->d[i] = d;
    prt->c[i] = c;

    prt->prt_npoint += 1;
}





/* load the conformation information of the prt */
void loadPrtConf(ProteinFile * prt_file, Protein0 * prt)
{
    /* load the protein */
    std::string p1_name = prt_file->path;
    prt_file->prt_npoint = 0; // why initialize since it is going to be overwriten
    prt_file->pnr = 0;
    prt_file->conf_total = 0;
    int prt_npoint = 0;
    int pnr = 0;
    int prt_conf = 0;

    Protein0 *myprt = &prt[prt_conf];	// address of the protein with conformation number prt_conf
    myprt->prt_npoint = 0;
    myprt->pnr = 0;

    list < string > p1_data;
    list < string >::iterator p1_i;

    string line1;

    ifstream p1_file(p1_name.c_str());


    if (!p1_file.is_open()) {
        cout << "cannot open protein file" << endl;
        cout << "Cannot open " << p1_name << endl;
        exit(EXIT_FAILURE);
    }

    while (getline(p1_file, line1))
        p1_data.push_back(line1);

    p1_file.close();

    std::string protein_seq1;	// original protein_seq1 in the class Complex
    // char protein_seq2[MAXPRO]; // original protein_seq2 in the class Complex

    /* load the total conf of prt */
    for (p1_i = p1_data.begin(); p1_i != p1_data.end(); p1_i++) {
        if ((*p1_i).substr(0, 6) == "ENDMDL") {
            prt_file->conf_total += 1;
        }
    }

    /* load indice of the protein */
    for (p1_i = p1_data.begin(); p1_i != p1_data.end(); p1_i++) {
        if ((*p1_i).size() > 53) {

            if ((*p1_i).substr(0, 6) == "ATOM  ") {

                std::string atom1 = (*p1_i).substr(12, 4);	// 3rd column
                int residue1 = getResCode((*p1_i).substr(17, 3));	// 4th column, residue name
                int residue2 = atoi((*p1_i).substr(22, 4).c_str());	// 5th column, residue serial number

                if (atom1 == " CA ") {

                    protein_seq1.append(three2oneS((*p1_i).substr(17, 3)));

                    myprt->seq3[pnr] = residue2;	// seq3 contains residue serial number

                    pushProteinPoint(myprt, pnr, prt_npoint++, 0, residue1, 0);

                    if (pnr > 1) {
                        pushProteinPoint(myprt, pnr - 1, prt_npoint++, 1, residue1, 1);
                    }

                    switch (residue1) {
                    case 0:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 2, residue1, 2);
                        break;
                    case 1:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 21, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 22, residue1, 3);
                        break;
                    case 2:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 3, residue1, 2);
                        break;
                    case 3:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 18, residue1, 2);
                        break;
                    case 4:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 14, residue1, 2);
                        break;
                    case 5:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 11, residue1, 2);
                        break;
                    case 6:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 5, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 6, residue1, 3);
                        break;
                    case 7:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 9, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 10, residue1, 3);
                        break;
                    case 8:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 26, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 27, residue1, 3);
                        break;
                    case 9:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 24, residue1, 2);
                        break;
                    case 10:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 15, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 16, residue1, 3);
                        break;
                    case 11:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 4, residue1, 2);
                        break;
                    case 13:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 7, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 8, residue1, 3);
                        break;
                    case 14:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 12, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 13, residue1, 3);
                        break;
                    case 15:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 19, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 20, residue1, 3);
                        break;
                    case 16:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 23, residue1, 2);
                        break;
                    case 17:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 17, residue1, 2);
                        break;
                    case 18:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 28, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 29, residue1, 3);
                        break;
                    case 19:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 25, residue1, 2);
                        break;
                    }

                    (myprt->pnr)++;

                    pnr++;
                }
            }
        }

        else if ((*p1_i).substr(0, 6) == "ENDMDL") {
            break;
        }
        // cout << "index part prt_conf: " << prt_conf << endl;
    }

    prt_file->prt_npoint = myprt->prt_npoint;	// assign the point number
    prt_file->pnr = myprt->pnr;	// assign the residue number
}




void loadProtein(ProteinFile * prt_file, Protein0 * prt)
{
    std::string p1_name = prt_file->path;
    int prt_conf = 0;
    int num_prt_conf = 0;	// max number of conformations in the data file
    Protein0 *myprt = &prt[prt_conf];	// address of the protein with conformation number prt_conf

    list < string > p1_data;
    list < string >::iterator p1_i;

    string line1;

    ifstream p1_file(p1_name.c_str());


    if (!p1_file.is_open()) {
        cout << "cannot open protein file" << endl;
        cout << "Cannot open " << p1_name << endl;
        exit(EXIT_FAILURE);
    }

    while (getline(p1_file, line1))
        p1_data.push_back(line1);

    p1_file.close();

    myprt->prt_npoint = 0;
    myprt->pnr = 0;
    int prt_npoint = 0;
    int pnr = 0;

    std::string protein_seq1;	// original protein_seq1 in the class Complex
    char protein_seq2[MAXPRO];	// original protein_seq2 in the class Complex

    for (p1_i = p1_data.begin(); p1_i != p1_data.end(); p1_i++) {
        if ((*p1_i).size() > 53) {

            if ((*p1_i).substr(0, 6) == "ATOM  ") {

                std::string atom1 = (*p1_i).substr(12, 4);	// 3rd column
                int residue1 = getResCode((*p1_i).substr(17, 3));	// 4th column, residue name
                int residue2 = atoi((*p1_i).substr(22, 4).c_str());	// 5th column, residue serial number

                if (atom1 == " CA ") {

                    protein_seq1.append(three2oneS((*p1_i).substr(17, 3)));

                    myprt->seq3[pnr] = residue2;	// seq3 contains residue serial number

                    pushProteinPoint(myprt, pnr, prt_npoint++, 0, residue1, 0);

                    if (pnr > 1) {
                        pushProteinPoint(myprt, pnr - 1, prt_npoint++, 1, residue1, 1);
                    }

                    switch (residue1) {
                    case 0:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 2, residue1, 2);
                        break;
                    case 1:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 21, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 22, residue1, 3);
                        break;
                    case 2:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 3, residue1, 2);
                        break;
                    case 3:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 18, residue1, 2);
                        break;
                    case 4:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 14, residue1, 2);
                        break;
                    case 5:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 11, residue1, 2);
                        break;
                    case 6:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 5, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 6, residue1, 3);
                        break;
                    case 7:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 9, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 10, residue1, 3);
                        break;
                    case 8:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 26, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 27, residue1, 3);
                        break;
                    case 9:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 24, residue1, 2);
                        break;
                    case 10:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 15, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 16, residue1, 3);
                        break;
                    case 11:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 4, residue1, 2);
                        break;
                    case 13:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 7, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 8, residue1, 3);
                        break;
                    case 14:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 12, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 13, residue1, 3);
                        break;
                    case 15:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 19, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 20, residue1, 3);
                        break;
                    case 16:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 23, residue1, 2);
                        break;
                    case 17:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 17, residue1, 2);
                        break;
                    case 18:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 28, residue1, 2);
                        pushProteinPoint(myprt, pnr, prt_npoint++, 29, residue1, 3);
                        break;
                    case 19:
                        pushProteinPoint(myprt, pnr, prt_npoint++, 25, residue1, 2);
                        break;
                    }

                    (myprt->pnr)++;

                    pnr++;
                }
            }
        }

        else if ((*p1_i).substr(0, 6) == "ENDMDL") {
            break;
        }
        // cout << "index part prt_conf: " << prt_conf << endl;
    }

    strcpy(protein_seq2, protein_seq1.c_str());

    for (int p2_i = 0; p2_i < (myprt->prt_npoint); p2_i++)	// iterating effective points
    {
        int residue3 = myprt->seq3[myprt->r[p2_i]];
        int residue5 = myprt->r[p2_i];
        int point1 = myprt->t[p2_i];

        // cout << "seq3[residue number]: " << residue3 << endl;
        // cout << "residue number: " << residue5 << endl;
        // cout << "point type: " << point1 << endl;

        float tx1 = 0.0;	// temperary x coord
        float ty1 = 0.0;	// temperary y coord
        float tz1 = 0.0;	// temperary z coord
        float tn1 = 0.0;	// ???, what is its use?

        for (p1_i = p1_data.begin(); p1_i != p1_data.end(); p1_i++) {
            //      cout << "--1--prt_conf " << prt_conf << endl;
            //      cout << "1-- prt_conf: " << prt_conf << endl;
            //              myprt = &prt[prt_conf]; // address of the protein with conformation number prt_conf

            //      cout << "beging search: " << prt_conf << endl;
            if ((*p1_i).size() > 53) {
                if ((*p1_i).substr(0, 6) == "ATOM  ") {
                    int residue4 = atoi((*p1_i).substr(22, 4).c_str());

                    std::string atom2 = (*p1_i).substr(12, 4);

                    float tx2 = atof((*p1_i).substr(30, 8).c_str());
                    float ty2 = atof((*p1_i).substr(38, 8).c_str());
                    float tz2 = atof((*p1_i).substr(46, 8).c_str());

                    //cout << "tz2 = " << "ASCII " << (*p1_i).substr (46, 8).c_str () << "float " << tz2 << endl;

                    if (residue4 == residue3) {
                        if (point1 == 0 && atom2 == " CA ") {
                            tx1 += tx2;
                            ty1 += ty2;
                            tz1 += tz2;

                            tn1 += 1.0;
                        } else if (point1 == 2
                            || point1 == 3
                            || point1 == 4
                            || point1 == 11
                            || point1 == 14
                            || point1 == 17
                            || point1 == 18
                            || point1 == 23 || point1 == 24 || point1 == 25) {
                            if (atom2 != " N  "
                                && atom2 != " CA " && atom2 != " C  " && atom2 != " O  ") {
                                tx1 += tx2;
                                ty1 += ty2;
                                tz1 += tz2;

                                tn1 += 1.0;
                            }
                        } else if (point1 == 5
                            || point1 == 7
                            || point1 == 9
                            || point1 == 12
                            || point1 == 15
                            || point1 == 19
                            || point1 == 21 || point1 == 26 || point1 == 28) {
                            if (atom2 != " N  "
                                && atom2 != " CA "
                                && atom2 != " C  "
                                && atom2 != " O  " && atom2 != " CB " && atom2 != " CG ") {
                                tx1 += tx2;
                                ty1 += ty2;
                                tz1 += tz2;

                                tn1 += 1.0;
                            }
                        } else if (point1 == 6
                            || point1 == 8
                            || point1 == 10
                            || point1 == 13
                            || point1 == 16
                            || point1 == 20
                            || point1 == 22 || point1 == 27 || point1 == 29) {
                            if (atom2 == " CB " || atom2 == " CG ") {
                                tx1 += tx2;
                                ty1 += ty2;
                                tz1 += tz2;

                                tn1 += 1.0;
                            }
                        }
                    }

                    if (point1 == 1 && residue5 > 0 && residue5 < pnr - 1) {
                        int residue6 = myprt->seq3[myprt->r[p2_i] - 1];

                        if ((residue4 == residue3 && atom2 == " N  ")
                            || (residue4 == residue6 && atom2 == " C  ")
                            || (residue4 == residue6 && atom2 == " O  ")) {
                            tx1 += tx2;
                            ty1 += ty2;
                            tz1 += tz2;

                            tn1 += 1.0;
                        }
                    }
                }
            } else if ((*p1_i).substr(0, 6) == "ENDMDL") {
                if (tn1 > 0.0) {
                    tx1 /= tn1;
                    ty1 /= tn1;
                    tz1 /= tn1;

                    myprt = &prt[prt_conf];	// address of the protein with conformation number prt_conf
                    myprt->x[p2_i] = tx1;	// set the x,y,z coords
                    myprt->y[p2_i] = ty1;
                    myprt->z[p2_i] = tz1;

                    /*

                       cout << "--3--prt_conf"  << prt_conf << endl;
                       cout << "x coord: " << myprt->x[p2_i] << endl;
                       cout << "y coord: " << myprt->y[p2_i] << endl;
                       cout << "z coord: " << myprt->z[p2_i] << endl;
                       */
                }

                tx1 = 0.0;
                ty1 = 0.0;
                tz1 = 0.0;
                tn1 = 0.0;

                prt_conf++;
                num_prt_conf = max(num_prt_conf, prt_conf);
                // num_prt = num_prt_conf;
                prt_file->conf_total = num_prt_conf;
            }
        }
        prt_conf = 0;
        myprt = &prt[prt_conf];
    }

    /* copy the point name and types from first conformation to the rest */
    for (int i = 1; i < num_prt_conf; i++) {

        Protein0 *myprt_rest = &prt[i];	// address of the protein with conformation number prt_conf
        myprt_rest->prt_npoint = 0;
        myprt_rest->pnr = 0;
        prt_npoint = 0;
        pnr = 0;

        for (p1_i = p1_data.begin(); p1_i != p1_data.end(); p1_i++) {
            if ((*p1_i).size() > 53) {

                if ((*p1_i).substr(0, 6) == "ATOM  ") {

                    std::string atom1 = (*p1_i).substr(12, 4);
                    int residue1 = getResCode((*p1_i).substr(17, 3));
                    int residue2 = atoi((*p1_i).substr(22, 4).c_str());

                    if (atom1 == " CA ") {

                        protein_seq1.append(three2oneS((*p1_i).substr(17, 3)));

                        myprt_rest->seq3[pnr] = residue2;

                        pushProteinPoint(myprt_rest, pnr, prt_npoint++, 0, residue1, 0);

                        if (pnr > 1) {
                            pushProteinPoint(myprt_rest, pnr - 1, prt_npoint++, 1, residue1, 1);
                        }

                        switch (residue1) {
                        case 0:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 2, residue1, 2);
                            break;
                        case 1:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 21, residue1, 2);
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 22, residue1, 3);
                            break;
                        case 2:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 3, residue1, 2);
                            break;
                        case 3:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 18, residue1, 2);
                            break;
                        case 4:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 14, residue1, 2);
                            break;
                        case 5:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 11, residue1, 2);
                            break;
                        case 6:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 5, residue1, 2);
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 6, residue1, 3);
                            break;
                        case 7:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 9, residue1, 2);
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 10, residue1, 3);
                            break;
                        case 8:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 26, residue1, 2);
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 27, residue1, 3);
                            break;
                        case 9:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 24, residue1, 2);
                            break;
                        case 10:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 15, residue1, 2);
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 16, residue1, 3);
                            break;
                        case 11:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 4, residue1, 2);
                            break;
                        case 13:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 7, residue1, 2);
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 8, residue1, 3);
                            break;
                        case 14:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 12, residue1, 2);
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 13, residue1, 3);
                            break;
                        case 15:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 19, residue1, 2);
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 20, residue1, 3);
                            break;
                        case 16:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 23, residue1, 2);
                            break;
                        case 17:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 17, residue1, 2);
                            break;
                        case 18:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 28, residue1, 2);
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 29, residue1, 3);
                            break;
                        case 19:
                            pushProteinPoint(myprt_rest, pnr, prt_npoint++, 25, residue1, 2);
                            break;
                        }

                        (myprt_rest->pnr)++;

                        pnr++;
                    }
                }
            } else if ((*p1_i).substr(0, 6) == "ENDMDL") {
                break;
            }
            // cout << "index part prt_conf: " << prt_conf << endl;
        }

    }

    prt_file->prt_npoint = myprt->prt_npoint;	// assign the point number
    prt_file->pnr = myprt->pnr;	// assign the residue number


}




void
pushKDEpoint (int at, float ax, float ay, float az, Kde0 * kde)
{
    int i = kde->kde_npoint;
    kde->kde_npoint += 1;

    kde->n[i] = i;
    kde->t[i] = at;
    kde->x[i] = ax;
    kde->y[i] = ay;
    kde->z[i] = az;

}






void
loadLHM (LhmFile * lhm_file, Psp0 * psp, Kde0 * kde, Mcs0 * mcs)
{
    for (int i = 0; i < MAX_MCS_ROW; ++i) {
        for (int j = 0; j < MAX_MCS_COL; ++j) {
            mcs[i].x[j] = MCS_INVALID_COORD;
            mcs[i].y[j] = MCS_INVALID_COORD;
            mcs[i].z[j] = MCS_INVALID_COORD;
        }
    }


    std::string h1_name = lhm_file->path;
    std::string ligand_id = lhm_file->ligand_id;



    string line1;

    int mcs_conf = 0;
    int num_mcs_conf = 0;		// max MCS constrains in .ff file
    Mcs0 *mymcs = &mcs[mcs_conf];
    kde->kde_npoint = 0;


    ifstream h1_file (h1_name.c_str ());

    if (!h1_file.is_open ()) {
        cout << "cannot open lhm file" << endl;
        cout << "Cannot open " << h1_name << endl;
        exit (EXIT_FAILURE);
    }

    while (getline (h1_file, line1)) {
        if (line1.length () > 3) {
            /* load KDE */
            if (line1.substr (0, 3) == "KDE") {
                std::string dat1[5];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                /* CoordsKDE(point number, atom type, x coord, y coord, z coord) */
                // cout << "0-- kde_npoint " << kde->kde_npoint << endl;
                pushKDEpoint (
                    getLigCode (dat1[1]),
                    atof (dat1[2].c_str ()),
                    atof (dat1[3].c_str ()),
                    atof (dat1[4].c_str ()),
                    kde);
                // cout << "1-- kde_npoint " << kde->kde_npoint << endl;

                // cout << "2-- kde->pns[getLigCode (dat1[1])] " << kde->pns[getLigCode (dat1[1])] << endl;
                kde->pns[getLigCode (dat1[1])]++;
                // cout << "3-- kde->pns[getLigCode (dat1[1])] " << kde->pns[getLigCode (dat1[1])] << endl;
            }
            /* load PSP */
            else if (line1.substr (0, 3) == "PSP") {
                std::string dat1[4];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                psp->psp[atoi (dat1[1].c_str ())][getLigCode (dat1[2])] =
                    atof (dat1[3].c_str ());
                psp->n += 1;	// BUG! n was not initialized
            }
            else if (line1.substr (0, 3) == "MCS") {

                mymcs = &mcs[mcs_conf];	// renew the pointer

#define MCS_STR_LENG 1024

                std::string dat1[MCS_STR_LENG];
                int dat2 = 0;
                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                if (dat1[1] == ligand_id) {
                    mymcs->tcc = atof (dat1[2].c_str ());

                    mymcs->ncol = atoi (dat1[3].c_str ());

                    for (int ia = 0; ia < mymcs->ncol; ia++) {
                        int mcs_loc = atoi (dat1[ia * 4 + 4].c_str ());
                        // performance optimization
                        // previously index start from 1
                        // now index start from 0
                        mcs_loc = mcs_loc - 1;
                        if (mcs_loc < 0 || mcs_loc >= MAXLIG - 1) {
                            cout << "error: MCS position overbound" << endl;
                            exit (EXIT_FAILURE);
                        }
                        mymcs->x[mcs_loc] = atof (dat1[ia * 4 + 5].c_str ());
                        mymcs->y[mcs_loc] = atof (dat1[ia * 4 + 6].c_str ());
                        mymcs->z[mcs_loc] = atof (dat1[ia * 4 + 7].c_str ());

                        mymcs->x2[ia] = atof (dat1[ia * 4 + 5].c_str ());
                        mymcs->y2[ia] = atof (dat1[ia * 4 + 6].c_str ());
                        mymcs->z2[ia] = atof (dat1[ia * 4 + 7].c_str ());
                        mymcs->idx_col[ia] = mcs_loc;
                    }

                    //        if (ligand_mcs.size () < (int) MAX_MCS_ROW)
                    //          ligand_mcs.push_back (tmp_mcs);
                }
                mcs_conf++;
                num_mcs_conf = max (num_mcs_conf, mcs_conf);
                // cout << "1-- mcs_conf " << mcs_conf << endl;
            }
        }

    }

    h1_file.close ();

    lhm_file->mcs_nrow = num_mcs_conf;

}











void
loadEnePara (EneParaFile * enepara_file, EnePara0 * enepara)
{

    string line1;

    ifstream d1_file (enepara_file->path.c_str ());

    if (!d1_file.is_open ()) {
        cout << "cannot open energy parameter file" << endl;
        cout << "Cannot open " << enepara_file->path << endl;
        exit (EXIT_FAILURE);
    }

    while (getline (d1_file, line1))
        if (line1.length () > 3) {
            if (line1.substr (0, 3) == "VDW") {
                std::string dat1[5];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                enepara->vdw[getPntCode (dat1[1])][getLigCode (dat1[2])][0] =
                    atof (dat1[3].c_str ());
                /**< for each pair of protein and ligand point, we have two parameters */
                enepara->vdw[getPntCode (dat1[1])][getLigCode (dat1[2])][1] =
                    atof (dat1[4].c_str ());
                /**< vdw[prtein pt][ligand pt][0] is the 1st parameter, and vdw[][][1] is the 2nd */
            }
            else if (line1.substr (0, 3) == "PLJ") {
                std::string dat1[4];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                for (int i1 = 0; i1 < 3; i1++)
                    enepara->lj[i1] = atof (dat1[i1 + 1].c_str ());
            }
            else if (line1.substr (0, 3) == "ELE") {
                std::string dat1[3];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                if (dat1[1].substr (1, 1) != "C")
                    enepara->ele[getPntCode (dat1[1])] = atof (dat1[2].c_str ());
                else
                    enepara->ele[getResCodeOne (dat1[1].substr (0, 1)) + 30] =
                        atof (dat1[2].c_str ());
            }
            else if (line1.substr (0, 3) == "PEL") {
                std::string dat1[3];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                for (int i1 = 0; i1 < 2; i1++)
                    enepara->el[i1] = atof (dat1[i1 + 1].c_str ());
            }
            else if (line1.substr (0, 3) == "PMF") {
                std::string dat1[5];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                enepara->pmf[getPntCode (dat1[1])][getLigCode (dat1[2])][0] =
                    atof (dat1[3].c_str ());
                enepara->pmf[getPntCode (dat1[1])][getLigCode (dat1[2])][1] =
                    atof (dat1[4].c_str ());
            }
            else if (line1.substr (0, 3) == "HPP") {
                std::string dat1[3];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                enepara->hpp[getResCodeOne (dat1[1])] = atof (dat1[2].c_str ());
            }
            else if (line1.substr (0, 3) == "HPL") {
                std::string dat1[4];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                enepara->hpl[getLigCode (dat1[1])][0] = atof (dat1[2].c_str ());
                enepara->hpl[getLigCode (dat1[1])][1] = atof (dat1[3].c_str ());
            }
            else if (line1.substr (0, 3) == "HDB") {
                std::string dat1[5];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                enepara->hdb[getPntCode (dat1[1])][getLigCode (dat1[2])][0] =
                    atof (dat1[3].c_str ());
                enepara->hdb[getPntCode (dat1[1])][getLigCode (dat1[2])][1] =
                    atof (dat1[4].c_str ());
            }
            else if (line1.substr (0, 3) == "WEI") {
                std::string dat1[10];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                for (int wi = 0; wi < MAXWEI; wi++) {
                    enepara->w[wi] = atof (dat1[wi + 1].c_str ());
                }

            }
            else if (line1.substr (0, 3) == "KDE") {
                std::string dat1[2];

                int dat2 = 0;

                istringstream dat3 (line1);

                while (dat3)
                    dat3 >> dat1[dat2++];

                enepara->kde = atof (dat1[1].c_str ());
            }

        }

    d1_file.close ();

    /* normalize hydrophobic scale */

    float hpc1 = 1e6;
    float hpc2 = -1e6;


    for (int ai = 0; ai < MAXTP4; ai++) {
        if (enepara->hpp[ai] < hpc1)
            hpc1 = enepara->hpp[ai];

        if (enepara->hpp[ai] > hpc2)
            hpc2 = enepara->hpp[ai];
    }

    for (int ai = 0; ai < MAXTP4; ai++) {
        enepara->hpp[ai] =
            ((enepara->hpp[ai] - hpc1) / (hpc2 - hpc1)) * 2.0 - 1.0;

        if (enepara->hpp[ai] < -1.0)
            enepara->hpp[ai] = -1.0;

        if (enepara->hpp[ai] > 1.0)
            enepara->hpp[ai] = 1.0;
    }

}

void loadNorPara(NorParaFile * norpara_file, EnePara0 * enepara )
{
    // loading parameter a
    std::string ifn = norpara_file->path_a;

    list < string > data;
    list < string >::iterator data_i;

    string line1;
    ifstream data_file(ifn.c_str());

    if (!data_file.is_open()) {
        cout << "cannot open nor a file" << endl;
        cout << "cannot open " << ifn << endl;
        exit(EXIT_FAILURE);
    }

    while (getline(data_file, line1))
        data.push_back(line1);

    data_file.close();

    int total_item = data.size();
    int iter = 0;

    for (iter = 0, data_i = data.begin(); iter < total_item && data_i != data.end(); iter++, data_i++) {
        string s = (*data_i).substr(0, 30);
        istringstream os(s);
        float tmp = 0.0;
        os >> tmp;
        enepara->a_para[iter] = tmp;
    }

    // loading parameter b
    std::string ifn2 = norpara_file->path_b;

    list < string > data2;
    list < string >::iterator data2_i;

    string line2;
    ifstream data2_file(ifn2.c_str());

    if (!data2_file.is_open()) {
        cout << "cannpt open nor b file" << endl;
        cout << "cannot open " << ifn2 << endl;
        exit(EXIT_FAILURE);
    }

    while (getline(data2_file, line2))
        data2.push_back(line2);

    data2_file.close();

    total_item = data2.size();
    iter = 0;

    for (iter = 0, data2_i = data2.begin(); iter < total_item && data2_i != data2.end(); iter++, data2_i++) {
        string s = (*data2_i).substr(0, 30);
        istringstream os(s);
        float tmp = 0.0;
        os >> tmp;
        enepara->b_para[iter] = tmp;
    }
}

void loadTrace(TraceFile * trace_file, float * trace)
{
    std::string ifn = trace_file->path;

    list < string > data;
    list < string >::iterator data_i;

    string line2;				// tmp string for each line
    ifstream data_file(ifn.c_str());	// open the data_file as the buffer

    if (!data_file.is_open()) {
        cout << "cannot open trace file" << endl;
        cout << "cannot open " << ifn << endl;
        exit(EXIT_FAILURE);
    }

    while (getline(data_file, line2))
        data.push_back(line2);	// push each line to the list

    data_file.close();			// close

    int total_trace_item = data.size();
    int trace_iter = 0;

    for (trace_iter = 0, data_i = data.begin(); trace_iter < total_trace_item && data_i != data.end(); trace_iter++, data_i++) {	// interate the list
        string s = (*data_i).substr(0, 30);
        istringstream os(s);
        float tmp = 0.0;
        os >> tmp;				// this tmp is what you need. do whatever you want with it
        trace[trace_iter] = tmp;
    }
}

void loadWeight(WeightFile * weight_file, EnePara0 * enepara )
{
    std::string ifn = weight_file->path;

    list < string > data;
    list < string >::iterator data_i;

    string line2;				// tmp string for each line
    ifstream data_file(ifn.c_str());	// open the data_file as the buffer

    if (!data_file.is_open()) {
        cout << "cannot open weight file" << endl;
        cout << "cannot open " << ifn << endl;
        exit(EXIT_FAILURE);
    }

    while (getline(data_file, line2))
        data.push_back(line2);	// push each line to the list

    data_file.close();			// close

    int total_weight_item = data.size();
    int weight_iter = 0;

    for (weight_iter = 0, data_i = data.begin(); weight_iter < total_weight_item && data_i != data.end(); weight_iter++, data_i++) {	// interate the list
        string s = (*data_i).substr(0, 30);
        istringstream os(s);
        float tmp = 0.0;
        os >> tmp;				// this tmp is what you need. do whatever you want with it
        enepara->w[weight_iter] = tmp;
    }
}
