/*!
 * \file definition_structure.cpp
 * \brief Main subroutines used by SU2_CFD
 * \author F. Palacios, T. Economon
 * \version 4.2.0 "Cardinal"
 *
 * SU2 Lead Developers: Dr. Francisco Palacios (Francisco.D.Palacios@boeing.com).
 *                      Dr. Thomas D. Economon (economon@stanford.edu).
 *
 * SU2 Developers: Prof. Juan J. Alonso's group at Stanford University.
 *                 Prof. Piero Colonna's group at Delft University of Technology.
 *                 Prof. Nicolas R. Gauger's group at Kaiserslautern University of Technology.
 *                 Prof. Alberto Guardone's group at Polytechnic University of Milan.
 *                 Prof. Rafael Palacios' group at Imperial College London.
 *
 * Copyright (C) 2012-2016 SU2, the open-source CFD code.
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/definition_structure.hpp"

unsigned short GetnZone(string val_mesh_filename, unsigned short val_format, CConfig *config) {
  string text_line, Marker_Tag;
  ifstream mesh_file;
  short nZone = 1; // Default value
  unsigned short iLine, nLine = 10;
  char cstr[200];
  string::size_type position;

  /*--- Search the mesh file for the 'NZONE' keyword. ---*/

  switch (val_format) {
    case SU2:

      /*--- Open grid file ---*/

      strcpy (cstr, val_mesh_filename.c_str());
      mesh_file.open(cstr, ios::in);
      if (mesh_file.fail()) {
        cout << "cstr=" << cstr << endl;
        cout << "There is no geometry file (GetnZone))!" << endl;

#ifndef HAVE_MPI
        exit(EXIT_FAILURE);
#else
        MPI_Abort(MPI_COMM_WORLD,1);
        MPI_Finalize();
#endif
      }

      /*--- Read the SU2 mesh file ---*/

      for (iLine = 0; iLine < nLine ; iLine++) {

        getline (mesh_file, text_line);

        /*--- Search for the "NZONE" keyword to see if there are multiple Zones ---*/

        position = text_line.find ("NZONE=",0);
        if (position != string::npos) {
          text_line.erase (0,6); nZone = atoi(text_line.c_str());
        }
      }

      break;

  }

  /*--- For time spectral integration, nZones = nTimeInstances. ---*/

  if (config->GetUnsteady_Simulation() == TIME_SPECTRAL) {
    nZone = config->GetnTimeInstances();
  }

  return (unsigned short) nZone;
}

unsigned short GetnDim(string val_mesh_filename, unsigned short val_format) {

  string text_line, Marker_Tag;
  ifstream mesh_file;
  short nDim = 3;
  unsigned short iLine, nLine = 10;
  char cstr[200];
  string::size_type position;

  /*--- Open grid file ---*/

  strcpy (cstr, val_mesh_filename.c_str());
  mesh_file.open(cstr, ios::in);

  switch (val_format) {
    case SU2:

      /*--- Read SU2 mesh file ---*/

      for (iLine = 0; iLine < nLine ; iLine++) {

        getline (mesh_file, text_line);

        /*--- Search for the "NDIM" keyword to see if there are multiple Zones ---*/

        position = text_line.find ("NDIME=",0);
        if (position != string::npos) {
          text_line.erase (0,6); nDim = atoi(text_line.c_str());
        }
      }
      break;

    case CGNS:

#ifdef HAVE_CGNS

      /*--- Local variables which are needed when calling the CGNS mid-level API. ---*/

      int fn, nbases = 0, nzones = 0, file_type;
      int cell_dim = 0, phys_dim = 0;
      char basename[CGNS_STRING_SIZE];

      /*--- Check whether the supplied file is truly a CGNS file. ---*/

      if ( cg_is_cgns(val_mesh_filename.c_str(), &file_type) != CG_OK ) {
        printf( "\n\n   !!! Error !!!\n" );
        printf( " %s is not a CGNS file.\n", val_mesh_filename.c_str());
        printf( " Now exiting...\n\n");
        exit(EXIT_FAILURE);
      }

      /*--- Open the CGNS file for reading. The value of fn returned
       is the specific index number for this file and will be
       repeatedly used in the function calls. ---*/

      if (cg_open(val_mesh_filename.c_str(), CG_MODE_READ, &fn)) cg_error_exit();

      /*--- Get the number of databases. This is the highest node
       in the CGNS heirarchy. ---*/

      if (cg_nbases(fn, &nbases)) cg_error_exit();

      /*--- Check if there is more than one database. Throw an
       error if there is because this reader can currently
       only handle one database. ---*/

      if ( nbases > 1 ) {
        printf("\n\n   !!! Error !!!\n" );
        printf("CGNS reader currently incapable of handling more than 1 database.");
        printf("Now exiting...\n\n");
        exit(EXIT_FAILURE);
      }

      /*--- Read the databases. Note that the indexing starts at 1. ---*/

      for ( int i = 1; i <= nbases; i++ ) {

        if (cg_base_read(fn, i, basename, &cell_dim, &phys_dim)) cg_error_exit();

        /*--- Get the number of zones for this base. ---*/

        if (cg_nzones(fn, i, &nzones)) cg_error_exit();

      }

      /*--- Set the problem dimension as read from the CGNS file ---*/

      nDim = cell_dim;

#endif

      break;

  }

  mesh_file.close();

  return (unsigned short) nDim;
}

void Driver_Preprocessing(CDriver **driver,
                          CConfig *config,
                          char* config_file_name,
                          unsigned short val_nZone,
                          unsigned short val_nDim,
                          bool fsi) {

  int rank = MASTER_NODE;
#ifdef HAVE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

  /*--- fsi implementations will use, as of now, BGS implentation. More to come. ---*/
  //bool fsi = config_container[ZONE_0]->GetFSI_Simulation();

  if (val_nZone == SINGLE_ZONE) {

    /*--- Single zone problem: instantiate the single zone driver class. ---*/
    if (rank == MASTER_NODE) cout << "Instantiating a single zone driver for the problem. " << endl;

    *driver = new CSingleZoneDriver(config_file_name, val_nZone, val_nDim);

  } else if (config->GetUnsteady_Simulation() == TIME_SPECTRAL) {

    /*--- Use the spectral method driver. ---*/

    if (rank == MASTER_NODE) cout << "Instantiating a spectral method driver for the problem. " << endl;
    *driver = new CSpectralDriver(config_file_name, val_nZone, val_nDim);

  } else if ((val_nZone == 2) && fsi) {

	    /*--- FSI problem: instantiate the FSI driver class. ---*/

	 if (rank == MASTER_NODE) cout << "Instantiating a Fluid-Structure Interaction driver for the problem. " << endl;
	 *driver = new CFSIDriver(config_file_name, val_nZone, val_nDim);

  } else {

    /*--- Multi-zone problem: instantiate the multi-zone driver class by default
     or a specialized driver class for a particular multi-physics problem. ---*/

    if (rank == MASTER_NODE) cout << "Instantiating a multi-zone driver for the problem. " << endl;
    *driver = new CMultiZoneDriver(config_file_name, val_nZone, val_nDim);

    /*--- Future multi-zone drivers instatiated here. ---*/

  }

}


void Partition_Analysis(CGeometry *geometry, CConfig *config) {

  /*--- This routine does a quick and dirty output of the total
   vertices, ghost vertices, total elements, ghost elements, etc.,
   so that we can analyze the partition quality. ---*/

  unsigned short nMarker = config->GetnMarker_All();
  unsigned short iMarker, iNodes, MarkerS, MarkerR;
  unsigned long iElem, iPoint, nVertexS, nVertexR;
  unsigned long nNeighbors = 0, nSendTotal = 0, nRecvTotal = 0;
  unsigned long nPointTotal=0, nPointGhost=0, nElemTotal=0;
  unsigned long nElemHalo=0, nEdge=0, nElemBound=0;
  int iRank;
  int rank = MASTER_NODE;
  int size = SINGLE_NODE;

#ifdef HAVE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

  nPointTotal = geometry->GetnPoint();
  nPointGhost = geometry->GetnPoint() - geometry->GetnPointDomain();
  nElemTotal  = geometry->GetnElem();
  nEdge       = geometry->GetnEdge();

  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    nElemBound  += geometry->GetnElem_Bound(iMarker);
    if ((config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) &&
        (config->GetMarker_All_SendRecv(iMarker) > 0)) {
      MarkerS = iMarker;  MarkerR = iMarker+1;
      nVertexS = geometry->nVertex[MarkerS];
      nVertexR = geometry->nVertex[MarkerR];
      nNeighbors++;
      nSendTotal += nVertexS;
      nRecvTotal += nVertexR;
    }
  }

  bool *isHalo = new bool[geometry->GetnElem()];
  for (iElem = 0; iElem < geometry->GetnElem(); iElem++) {
    isHalo[iElem] = false;
    for (iNodes = 0; iNodes < geometry->elem[iElem]->GetnNodes(); iNodes++) {
      iPoint = geometry->elem[iElem]->GetNode(iNodes);
      if (!geometry->node[iPoint]->GetDomain()) isHalo[iElem] = true;
    }
  }

  for (iElem = 0; iElem < geometry->GetnElem(); iElem++) {
    if (isHalo[iElem]) nElemHalo++;
  }

  unsigned long *row_ptr = NULL, nnz;
  unsigned short *nNeigh = NULL;
  vector<unsigned long>::iterator it;
  vector<unsigned long> vneighs;

  /*--- Don't delete *row_ptr, *col_ind because they are
   asigned to the Jacobian structure. ---*/

  /*--- Compute the number of neighbors ---*/

  nNeigh = new unsigned short [geometry->GetnPoint()];
  for (iPoint = 0; iPoint < geometry->GetnPoint(); iPoint++) {
    // +1 -> to include diagonal element
    nNeigh[iPoint] = (geometry->node[iPoint]->GetnPoint()+1);
  }

  /*--- Create row_ptr structure, using the number of neighbors ---*/

  row_ptr = new unsigned long [geometry->GetnPoint()+1];
  row_ptr[0] = 0;
  for (iPoint = 0; iPoint < geometry->GetnPoint(); iPoint++)
    row_ptr[iPoint+1] = row_ptr[iPoint] + nNeigh[iPoint];
  nnz = row_ptr[geometry->GetnPoint()];

  delete [] row_ptr;
  delete [] nNeigh;

  /*--- Now put this info into a CSV file for processing ---*/

  char cstr[200];
  ofstream Profile_File;
  strcpy (cstr, "partitioning.csv");
  Profile_File.precision(15);

  if (rank == MASTER_NODE) {
    /*--- Prepare and open the file ---*/
    Profile_File.open(cstr, ios::out);
    /*--- Create the CSV header ---*/
    Profile_File << "\"Rank\", \"nNeighbors\", \"nPointTotal\", \"nEdge\", \"nPointGhost\", \"nSendTotal\", \"nRecvTotal\", \"nElemTotal\", \"nElemBoundary\", \"nElemHalo\", \"nnz\"" << endl;
    Profile_File.close();
  }
#ifdef HAVE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  /*--- Loop through the map and write the results to the file ---*/

  for (iRank = 0; iRank < size; iRank++) {
    if (rank == iRank) {
      Profile_File.open(cstr, ios::out | ios::app);
      Profile_File << rank << ", " << nNeighbors << ", " << nPointTotal << ", " << nEdge << "," << nPointGhost << ", " << nSendTotal << ", " << nRecvTotal << ", " << nElemTotal << "," << nElemBound << ", " << nElemHalo << ", " << nnz << endl;
      Profile_File.close();
    }
#ifdef HAVE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
  }

  delete [] isHalo;

}
