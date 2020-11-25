//
// Copyright (c) 2019-2020 CNRS INRIA
//

#include <boost/python.hpp>

#include "pinocchio/bindings/python/fwd.hpp"
#include "pinocchio/bindings/python/utils/namespace.hpp"
#include "pinocchio/math/rpy.hpp"

namespace pinocchio
{
  namespace python
  {
    namespace bp = boost::python;

    Eigen::Matrix3d rpyToMatrix_proxy(const Eigen::Vector3d & rpy)
    {
      return pinocchio::rpy::rpyToMatrix(rpy);
    }

    BOOST_PYTHON_FUNCTION_OVERLOADS(rpyToJac_overload, rpy::rpyToJac, 1, 2)
    BOOST_PYTHON_FUNCTION_OVERLOADS(rpyToJacInv_overload, rpy::rpyToJacInv, 1, 2)
    BOOST_PYTHON_FUNCTION_OVERLOADS(rpyToJacDerivative_overload, rpy::rpyToJacDerivative, 2, 3)

    Eigen::Matrix3d rotate(const std::string & axis, const double ang)
    {
      if(axis.length() != 1U)
          throw std::invalid_argument(std::string("Invalid axis: ").append(axis));
      Eigen::Vector3d u;
      u.setZero();
      const char axis_ = axis[0];
      switch(axis_)
      {
        case 'x': u[0] = 1.; break;
        case 'y': u[1] = 1.; break;
        case 'z': u[2] = 1.; break;
        default: throw std::invalid_argument(std::string("Invalid axis: ").append(1U,axis_));
      }

      return Eigen::AngleAxisd(ang, u).matrix();
    }

    void exposeRpy()
    {
      using namespace Eigen;
      using namespace pinocchio::rpy;

      {
        // using the rpy scope
        bp::scope current_scope = getOrCreatePythonNamespace("rpy");

        bp::def("rpyToMatrix",
                static_cast<Matrix3d (*)(const double, const double, const double)>(&rpyToMatrix),
                bp::args("roll", "pitch", "yaw"),
                "Given (r, p, y), the rotation is given as R = R_z(y)R_y(p)R_x(r),"
                " where R_a(theta) denotes the rotation of theta radians axis a");

        bp::def("rpyToMatrix",
                &rpyToMatrix_proxy,
                bp::arg("rpy"),
                "Given (r, p, y), the rotation is given as R = R_z(y)R_y(p)R_x(r),"
                " where R_a(theta) denotes the rotation of theta radians axis a");

        bp::def("matrixToRpy",
                &matrixToRpy<Matrix3d>,
                bp::arg("R"),
                "Given a rotation matrix R, the angles (r, p, y) are given so that R = R_z(y)R_y(p)R_x(r),"
                " where R_a(theta) denotes the rotation of theta radians axis a."
                " The angles are guaranteed to be in the ranges: r in [-pi,pi],"
                " p in[-pi/2,pi/2], y in [-pi,pi]");

        bp::def("rotate",
                &rotate,
                bp::args("axis", "ang"),
                "Rotation matrix corresponding to a rotation about x, y or z"
                " e.g. R = rot('x', pi / 4): rotate pi/4 rad about x axis");

        bp::def("rpyToJac",
                &rpyToJac<Vector3d>,
                rpyToJac_overload(
                    bp::args("rpy","reference_frame"),
                    "Compute the Jacobian of the Roll-Pitch-Yaw conversion"
                    " Given phi = (r, p, y) such that that R = R_z(y)R_y(p)R_x(r)"
                    " and reference frame F (either LOCAL or WORLD),"
                    " the Jacobian is such that omega_F = J_F(phi)phidot,"
                    " where omega_F is the angular velocity expressed in frame F"
                    " and J_F is the Jacobian computed with reference frame F"
                    "\nParameters:\n"
                    "\trpy Roll-Pitch-Yaw vector"
                    "\treference_frame  Reference frame in which the angular velocity is expressed."
                    " Notice LOCAL_WORLD_ALIGNED is equivalent to WORLD"
                )
        );

        bp::def("rpyToJacInv",
                &rpyToJacInv<Vector3d>,
                rpyToJacInv_overload(
                    bp::args("rpy","reference_frame"),
                    "Compute the inverse Jacobian of the Roll-Pitch-Yaw conversion"
                    " Given phi = (r, p, y) such that that R = R_z(y)R_y(p)R_x(r)"
                    " and reference frame F (either LOCAL or WORLD),"
                    " the Jacobian is such that omega_F = J_F(phi)phidot,"
                    " where omega_F is the angular velocity expressed in frame F"
                    " and J_F is the Jacobian computed with reference frame F"
                    "\nParameters:\n"
                    "\trpy Roll-Pitch-Yaw vector"
                    "\treference_frame  Reference frame in which the angular velocity is expressed."
                    " Notice LOCAL_WORLD_ALIGNED is equivalent to WORLD"
                )
        );

        bp::def("rpyToJacDerivative",
                &rpyToJacDerivative<Vector3d, Vector3d>,
                rpyToJacDerivative_overload(
                    bp::args("rpy", "rpydot", "reference_frame"),
                    "Compute the time derivative of the Jacobian of the Roll-Pitch-Yaw conversion"
                    " Given phi = (r, p, y) such that that R = R_z(y)R_y(p)R_x(r)"
                    " and reference frame F (either LOCAL or WORLD),"
                    " the Jacobian is such that omega_F = J_F(phi)phidot,"
                    " where omega_F is the angular velocity expressed in frame F"
                    " and J_F is the Jacobian computed with reference frame F"
                    "\nParameters:\n"
                    "\trpy Roll-Pitch-Yaw vector"
                    "\treference_frame  Reference frame in which the angular velocity is expressed."
                    " Notice LOCAL_WORLD_ALIGNED is equivalent to WORLD"
                )
        );
      }
      
    }
    
  } // namespace python
} // namespace pinocchio
