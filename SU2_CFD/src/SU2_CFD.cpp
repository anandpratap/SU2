#include "../include/SU2_CFD.hpp"

using namespace std;

int main(int argc, char *argv[]){

    char config_file_name[MAX_STRING_SIZE];
    unsigned short nDim, nZone;
    bool fsi;

    /*--- MPI initialization, and buffer setting ---*/

#ifdef HAVE_MPI
    int *bptr, bl;
    SU2_MPI::Init(&argc, &argv);
    MPI_Buffer_attach( malloc(BUFSIZE), BUFSIZE );
#endif // HAVE_MPI

    /*--- Create a pointer to the main SU2 Driver ---*/
    CDriver *driver = NULL;

    /*--- Load in the number of zones and spatial dimensions in the mesh file (If no config
   file is specified, default.cfg is used) ---*/

    if (argc == 2) { strcpy(config_file_name, argv[1]); }
    else { strcpy(config_file_name, "default.cfg"); }

    /*--- Read the name and format of the input mesh file to get from the mesh
   file the number of zones and dimensions from the numerical grid (required
   for variables allocation)  ---*/

    CConfig *config = NULL;
    config = new CConfig(config_file_name, SU2_CFD);

    nZone = GetnZone(config->GetMesh_FileName(), config->GetMesh_FileFormat(), config);
    nDim  = GetnDim(config->GetMesh_FileName(), config->GetMesh_FileFormat());
    fsi = config->GetFSI_Simulation();

    /*--- First, given the basic information about the number of zones and the
   solver types from the config, instantiate the appropriate driver for the problem
   and perform all the preprocessing. ---*/

    if (nZone == SINGLE_ZONE) {

    /*--- Single zone problem: instantiate the single zone driver class. ---*/

    driver = new CSingleZoneDriver(config_file_name, nZone, nDim);

  } else if (config->GetUnsteady_Simulation() == TIME_SPECTRAL) {

    /*--- Use the spectral method driver. ---*/

    driver = new CSpectralDriver(config_file_name, nZone, nDim);

  } else if ((nZone == 2) && fsi) {

	    /*--- FSI problem: instantiate the FSI driver class. ---*/

	 driver = new CFSIDriver(config_file_name, nZone, nDim);

  } else {

    /*--- Multi-zone problem: instantiate the multi-zone driver class by default
     or a specialized driver class for a particular multi-physics problem. ---*/

    driver = new CMultiZoneDriver(config_file_name, nZone, nDim);

    /*--- Future multi-zone drivers instatiated here. ---*/

  }

    delete config;
    config = NULL;

    /*--- Launch the main external loop of the solver ---*/
    driver->StartSolver();

    /*--- Postprocess all the containers, close history file, exit SU2 ---*/
    driver->Postprocessing();

    if(driver != NULL) delete driver;
    driver = NULL;

#ifdef HAVE_MPI
  /*--- Finalize MPI parallelization ---*/
  MPI_Buffer_detach(&bptr, &bl);
  MPI_Finalize();
#endif // HAVE_MPI

    return EXIT_SUCCESS;
}
