#ifndef  DOCK_SOA_H
#define  DOCK_SOA_H

// struct of array



struct MYALIGN Ligand
{
  // coord_xyz "orig" is under the ligand_ceter system
  float x[MAXLIG];		// Ligand x coords, optimized
  float y[MAXLIG];		// Ligand y coords, optimized
  float z[MAXLIG];		// Ligand z coords, optimized

  // xyz with the same order from Ligand0, no rearrange, for MCS computation
  float x2[MAXLIG];
  float y2[MAXLIG];
  float z2[MAXLIG];

  // xyz sort by t
  float x3[MAXLIG];
  float y3[MAXLIG];
  float z3[MAXLIG];
  int t3[MAXLIG];

  int t[MAXLIG];		// atom type, index, might sort ligand point by t
  float c[MAXLIG];		// atom charge, optimized



  int kde_begin_idx[MAXLIG];
  int kde_end_idx[MAXLIG];

  // coord_center is under the lab system
  float center[3];		// ligand geometric center

  int lig_natom;			// number of ligand atoms
};







struct MYALIGN Protein
{
  // the order reflect the reference order from calc energy function

  int t[MAXPRO];		// effective point type, index, might sort prt points by t
  float x[MAXPRO];		// residue x coord, optimized
  float y[MAXPRO];		// residue y coord, optimized
  float z[MAXPRO];		// residue z coord, optimized

  int cdc[MAXPRO];              // might sort
  float hpp[MAXPRO];            // enepara->hpp[prt->d[i]], optimized
  float ele[MAXPRO];            // dt = prt->t[i] == 0 ? prt->d[i] + 30 : prt->t[i];
                                // enepara->ele[dt], optimized
  int c[MAXPRO];		// effective point class, might sort prt point by c
  int seq3r[MAXPRO];            // index, might sort


  float pocket_center[3];

  int prt_npoint;			// number of protein effective points
};




struct MYALIGN Psp
{
  float psp[MAXLIG][MAXPRO];                                           // replaced
  //float sparse_psp[MAXLIG][MAXPRO];                                    // replaced
};



struct MYALIGN Kde
{
  int t[MAXKDE];		// KDE atom type, sort kde point by t
  float x[MAXKDE];		// KDE x coord, optimized
  float y[MAXKDE];		// KDE y coord, optimized
  float z[MAXKDE];		// KDE z coord, optimized

  int kde_npoint;			// number of kde points
};



struct MYALIGN Mcs
{
  float x[MAX_MCS_COL];              //                                      used
  float y[MAX_MCS_COL];              //                                      used
  float z[MAX_MCS_COL];              //                                      used


/*
  // sparse matrix, Jagged Diagonal Storage (JDS)
  int   idx_col[MAX_MCS_COL];              // index for sparse matrix              used
  float x2[MAX_MCS_COL];              //                                      used
  float y2[MAX_MCS_COL];              //                                      used
  float z2[MAX_MCS_COL];              //                                      used
*/
  float tcc;                    //                                      used
  int ncol;
};


struct MYALIGN Mcs2
{
  float x[MAX_MCS_ROW][MAX_MCS_COL];
  float y[MAX_MCS_ROW][MAX_MCS_COL];
  float z[MAX_MCS_ROW][MAX_MCS_COL];
  float tcc[MAX_MCS_ROW];
};


struct MYALIGN Mcs3
{
  float x[MAX_MCS_COL][MAX_MCS_ROW];
  float y[MAX_MCS_COL][MAX_MCS_ROW];
  float z[MAX_MCS_COL][MAX_MCS_ROW];
  float tcc[MAX_MCS_ROW];
};


// sparse matrix, CSR
struct MYALIGN Mcs4
{
  int idx_col[MAX_MCS_COL * MAX_MCS_ROW];
  float x[MAX_MCS_COL * MAX_MCS_ROW];
  float y[MAX_MCS_COL * MAX_MCS_ROW];
  float z[MAX_MCS_COL * MAX_MCS_ROW];
  float tcc[MAX_MCS_ROW];


  int row_ptr[MAX_MCS_ROW]; // the row pointer for CSR sparse matrix
  int ncol[MAX_MCS_ROW];
  int nrow;
  int npoint;
};



typedef Mcs4 McsM;





#if 1
struct MYALIGN EnePara
{
  // L-J
  float p1a[MAXTP2][MAXTP1];
  float p2a[MAXTP2][MAXTP1];
  float lj0, lj1;

  // electrostatic
  float el1;
  float el0;
  float a1; // 4.0f - 3.0f * el0;
  float b1; // 2.0f * el0 - 3.0f;

  // contact
  float pmf0[MAXTP2][MAXTP1];  // interchange to [lig][prt]
  float pmf1[MAXTP2][MAXTP1];  // interchange to [lig][prt]
  float hdb0[MAXTP2][MAXTP1];  // interchange to [lig][prt]
  float hdb1[MAXTP2][MAXTP1];  // interchange to [lig][prt]

  // hydrophobic
  float hpp[MAXTP4];
  float hpl0[MAXTP2];
  float hpl1[MAXTP2];
  float hpl2[MAXTP2];

  // kde
  float kde2; // -0.5f / (kde * kde)
  float kde3; // powf (kde * sqrtf (2.0f * PI), 3.0f)

  // weights for energy terms
  float a_para[MAXWEI];         // the a parameter for normalization
  float b_para[MAXWEI];         // the b parameter for normalization
  float w[MAXWEI];
};
#endif


#if 0
struct MYALIGN EnePara
{
  // L-J
  float p1a[MAXTP1][MAXTP2];
  float p2a[MAXTP1][MAXTP2];
  float lj0, lj1;

  // electrostatic
  float el1;
  float el0;
  float a1; // 4.0f - 3.0f * el0;
  float b1; // 2.0f * el0 - 3.0f;

  // contact
  float pmf0[MAXTP1][MAXTP2];
  float pmf1[MAXTP1][MAXTP2];
  float hdb0[MAXTP1][MAXTP2];
  float hdb1[MAXTP1][MAXTP2];

  // hydrophobic
  float hpp[MAXTP4];
  float hpl0[MAXTP2];
  float hpl1[MAXTP2];
  float hpl2[MAXTP2];

  // kde
  float kde2; // -0.5f / (kde * kde)
  float kde3; // powf (kde * sqrtf (2.0f * PI), 3.0f)

  // weights for energy terms
  float a_para[MAXWEI];         // the a parameter for normalization
  float b_para[MAXWEI];         // the b parameter for normalization
  float w[MAXWEI];
};
#endif





#endif // DOCK_SOA_H

