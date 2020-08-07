//
// Copyright (c) 2019-2020 INRIA
//

#include "pinocchio/algorithm/jacobian.hpp"
#include "pinocchio/algorithm/frames.hpp"
#include "pinocchio/algorithm/kinematics.hpp"
#include "pinocchio/algorithm/rnea.hpp"
#include "pinocchio/algorithm/rnea-derivatives.hpp"
#include "pinocchio/algorithm/impulse-dynamics.hpp"
#include "pinocchio/algorithm/impulse-dynamics-derivatives.hpp"
#include "pinocchio/algorithm/joint-configuration.hpp"
#include "pinocchio/parsers/sample-models.hpp"

#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/utility/binary.hpp>

BOOST_AUTO_TEST_SUITE(BOOST_TEST_MODULE)

using namespace Eigen;
using namespace pinocchio;

BOOST_AUTO_TEST_CASE(test_sparse_impulse_dynamics_derivatives_no_contact)
{
  //result: (dMdq)(dqafter-v) = drnea(q,0,dqafter-v)
  using namespace Eigen;
  using namespace pinocchio;

  Model model;
  buildModels::humanoidRandom(model,true);
  Data data(model), data_ref(model);
  
  model.lowerPositionLimit.head<3>().fill(-1.);
  model.upperPositionLimit.head<3>().fill( 1.);
  VectorXd q = randomConfiguration(model);

  VectorXd v = VectorXd::Random(model.nv);
  
  // Contact models and data
  const PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidContactModel) empty_contact_models;
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidContactData) empty_contact_data;

  const double mu0 = 0.;
  const double r_coeff = 0.5;
  
  initContactDynamics(model,data,empty_contact_models);
  impulseDynamics(model,data,q,v,empty_contact_models,empty_contact_data,r_coeff, mu0);

  const Eigen::VectorXd dv = data.dq_after-v;
  computeImpulseDynamicsDerivatives(model,data,empty_contact_models,empty_contact_data,r_coeff,mu0);

  Motion gravity_bk = model.gravity;
  model.gravity.setZero();
  computeRNEADerivatives(model, data_ref, q, Eigen::VectorXd::Zero(model.nv), dv);
  // Reference values
  BOOST_CHECK(data_ref.dtau_dq.isApprox(data.dtau_dq));
}

BOOST_AUTO_TEST_CASE(test_sparse_impulse_dynamics_derivatives)
{
  using namespace Eigen;
  using namespace pinocchio;
  Model model;
  buildModels::humanoidRandom(model,true);
  Data data(model), data_ref(model);
  
  model.lowerPositionLimit.head<3>().fill(-1.);
  model.upperPositionLimit.head<3>().fill( 1.);
  VectorXd q = randomConfiguration(model);
  VectorXd v = VectorXd::Random(model.nv);
  
  const std::string RF = "rleg6_joint";
  //  const Model::JointIndex RF_id = model.getJointId(RF);
  const std::string LF = "lleg6_joint";
  //  const Model::JointIndex LF_id = model.getJointId(LF);
  
  // Contact models and data
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidContactModel) contact_models;
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidContactData) contact_data;


  RigidContactModel ci_LF(CONTACT_6D,model.getFrameId(LF),WORLD);
  RigidContactModel ci_RF(CONTACT_3D,model.getFrameId(RF),WORLD);

  contact_models.push_back(ci_LF); contact_data.push_back(RigidContactData(ci_LF));
  contact_models.push_back(ci_RF); contact_data.push_back(RigidContactData(ci_RF));

  Eigen::DenseIndex constraint_dim = 0;
  for(size_t k = 0; k < contact_models.size(); ++k)
    constraint_dim += contact_models[k].size();
  
  const double mu0 = 0.;
  const double r_coeff = 0.5;
  
  initContactDynamics(model,data,contact_models);
  impulseDynamics(model,data,q,v,contact_models,contact_data,r_coeff,mu0);
  computeImpulseDynamicsDerivatives(model,data,contact_models,contact_data,r_coeff,mu0);
  
  typedef PINOCCHIO_ALIGNED_STD_VECTOR(Force) ForceVector;

  ForceVector iext((size_t)model.njoints);
  for(ForceVector::iterator it = iext.begin(); it != iext.end(); ++it)
    (*it).setZero();
  
  iext[model.getJointId(LF)] = data.oMi[model.getJointId(LF)].actInv(contact_data[0].contact_force);
  iext[model.getJointId(RF)] = data.oMi[model.getJointId(RF)].actInv(contact_data[1].contact_force);
  //iext[model.getJointId(LF)] = contact_data[0].contact_force;
  //iext[model.getJointId(RF)] = contact_data[1].contact_force;  

  Eigen::VectorXd effective_v = (1+r_coeff)*v+data.ddq;

  computeForwardKinematicsDerivatives(model, data_ref, q, effective_v,
                                      Eigen::VectorXd::Zero(model.nv));
  
  //TODO: Check why BOOST_CHECK(data_ref.ov[i].isApprox((1+r_coeff)*data.ov[i]+data.oa[i]))
  //      fails for rf and lf joints. the values are good! If we set data.ddq.setRandom,
  //      everything is okay. isApprox fails if one of the vectors is very close to zero
  for(int i=0;i<data.ov.size();i++){
    BOOST_CHECK(((1+r_coeff)*data.ov[i]+data.oa[i]
                 - data_ref.ov[i]).toVector().norm()<=1e-12);
  }
  
  Eigen::MatrixXd Jc(9,model.nv), dv_dq(9,model.nv), Jc_tmp(6,model.nv), dv_dq_tmp(6,model.nv);
  Jc.setZero(); dv_dq.setZero(); dv_dq_tmp.setZero(); Jc_tmp.setZero();

  getJointVelocityDerivatives(model, data_ref, model.getJointId(LF),WORLD,
                              dv_dq.topRows<6>(), Jc.topRows<6>());

  getJointVelocityDerivatives(model, data_ref, model.getJointId(RF),WORLD,
                              dv_dq_tmp, Jc_tmp);

  Jc.bottomRows<3>() = Jc_tmp.topRows<3>();
  dv_dq.bottomRows<3>() = dv_dq_tmp.topRows<3>();

  BOOST_CHECK((data_ref.J-data.J).norm() <=1e-12);

  Motion gravity_bk = model.gravity;
  model.gravity.setZero();
  computeRNEADerivatives(model, data_ref, q, Eigen::VectorXd::Zero(model.nv),
                         data.ddq, iext);
  model.gravity = gravity_bk;

  BOOST_CHECK(data.dac_da.isApprox(Jc));
  BOOST_CHECK(data.dtau_dq.isApprox(data_ref.dtau_dq-Jc.transpose()*data.dlambda_dq));
  BOOST_CHECK((data.dvc_dq-(dv_dq-Jc*data.Minv*data_ref.dtau_dq)).norm()<=1e-12);

  BOOST_CHECK((data.dlambda_dv+(1+r_coeff)*data.osim*Jc).norm()<=1e-12);
  
}

BOOST_AUTO_TEST_CASE ( test_impulse_dynamics_derivatives_fd )
{
  using namespace Eigen;
  using namespace pinocchio;

  Model model;
  buildModels::humanoidRandom(model,true);
  Data data(model), data_fd(model);

  model.lowerPositionLimit.head<3>().fill(-1.);
  model.upperPositionLimit.head<3>().fill( 1.);
  VectorXd q = randomConfiguration(model);

  VectorXd v = VectorXd::Random(model.nv);

  const std::string RF = "rleg6_joint";
  //  const Model::JointIndex RF_id = model.getJointId(RF);
  const std::string LF = "lleg6_joint";
  //  const Model::JointIndex LF_id = model.getJointId(LF);

  // Contact models and data
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidContactModel) contact_models;
  PINOCCHIO_STD_VECTOR_WITH_EIGEN_ALLOCATOR(RigidContactData) contact_data;

  RigidContactModel ci_LF(CONTACT_6D,model.getFrameId(LF),WORLD);
  RigidContactModel ci_RF(CONTACT_3D,model.getFrameId(RF),WORLD);

  contact_models.push_back(ci_LF); contact_data.push_back(RigidContactData(ci_LF));
  contact_models.push_back(ci_RF); contact_data.push_back(RigidContactData(ci_RF));

  Eigen::DenseIndex constraint_dim = 0;
  for(size_t k = 0; k < contact_models.size(); ++k)
    constraint_dim += contact_models[k].size();

  const double mu0 = 0.;
  const double r_coeff = 0.5;

  initContactDynamics(model,data,contact_models);
  impulseDynamics(model,data,q,v,contact_models,contact_data,r_coeff,mu0);
  computeImpulseDynamicsDerivatives(model, data, contact_models, contact_data, r_coeff, mu0);
  
  //Data_fd
  initContactDynamics(model,data_fd,contact_models);

  MatrixXd dqafter_partial_dq_fd(model.nv,model.nv); dqafter_partial_dq_fd.setZero();
  MatrixXd dqafter_partial_dv_fd(model.nv,model.nv); dqafter_partial_dv_fd.setZero();

  MatrixXd impulse_partial_dq_fd(constraint_dim,model.nv); impulse_partial_dq_fd.setZero();
  MatrixXd impulse_partial_dv_fd(constraint_dim,model.nv); impulse_partial_dv_fd.setZero();

  const VectorXd ddv0 = impulseDynamics(model,data_fd,q,v,contact_models,
                                        contact_data,r_coeff,mu0);
  const VectorXd impulse0 = data_fd.impulse_c;
  VectorXd v_eps(VectorXd::Zero(model.nv));
  VectorXd q_plus(model.nq);
  VectorXd ddv_plus(model.nv);

  VectorXd impulse_plus(constraint_dim);
  
  const double alpha = 1e-8;
  for(int k = 0; k < model.nv; ++k)
  {
    v_eps[k] += alpha;
    q_plus = integrate(model,q,v_eps);
    ddv_plus = impulseDynamics(model,data_fd,q_plus,v,contact_models,contact_data,r_coeff,mu0);
    dqafter_partial_dq_fd.col(k) = (ddv_plus - ddv0)/alpha;
    impulse_partial_dq_fd.col(k) = (data_fd.impulse_c - impulse0)/alpha;

    v_eps[k] = 0.;
  }
  
  BOOST_CHECK(dqafter_partial_dq_fd.isApprox(data.ddq_dq,sqrt(alpha)));
  BOOST_CHECK(impulse_partial_dq_fd.isApprox(data.dlambda_dq,sqrt(alpha)));

  BOOST_CHECK(dqafter_partial_dq_fd.isApprox(-data.Minv*(data.dtau_dq + data.dac_da.transpose()*(data.dlambda_dq - impulse_partial_dq_fd)),sqrt(alpha)));
  
  VectorXd v_plus(v);
  for(int k = 0; k < model.nv; ++k)
  {
    v_plus[k] += alpha;
    ddv_plus = impulseDynamics(model,data_fd,q,v_plus,contact_models,contact_data,r_coeff,mu0);

    dqafter_partial_dv_fd.col(k) = (ddv_plus - ddv0)/alpha;
    v_plus[k] -= alpha;
  }
  
  BOOST_CHECK(dqafter_partial_dv_fd.isApprox(Eigen::MatrixXd::Identity(model.nv,model.nv)+data.ddq_dv,sqrt(alpha)));
}

BOOST_AUTO_TEST_SUITE_END ()
