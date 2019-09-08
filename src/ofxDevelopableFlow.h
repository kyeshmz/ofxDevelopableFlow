//local developable files
#include "ofxDevelopableMesh.h"
#include "ofxDevelopableTypes.h"

//other external dependencies
#include "ofxEigen.h"
#include "ofxLibigl.h"


using namespace Eigen;
#include <developableflow/compute_cut.h>
#include <developableflow/curvature_energy.h>
#include <developableflow/energy_selector.h>
#include <developableflow/exactfcts_bisectors.h>
#include <developableflow/flatten_cut.h>
#include <developableflow/hinge_energy.h>
#include <developableflow/hingepairs_energy.h>
#include <developableflow/max_hinge_energy.h>
#include <developableflow/measure_once_cut_twice.h>
//#include <developableflow/mesh_postprocessing.h>
#include <developableflow/old_hinge_energy.h>
#include <developableflow/old_max_hinge_energy.h>
#include <developableflow/timestep.h>



#include <tools/triangle_cosdihedral_angle.h>
#include <tools/qc_color.h>
#include <tools/write_energy.h>
#include <tools/write_cut_meshes.h>
#include <tools/numerical_gradient.h>
#include <tools/perturb.h>