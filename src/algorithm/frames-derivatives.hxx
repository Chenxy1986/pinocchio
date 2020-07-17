//
// Copyright (c) 2020 INRIA
//

#ifndef __pinocchio_algorithm_frames_derivatives_hxx__
#define __pinocchio_algorithm_frames_derivatives_hxx__

#include "pinocchio/multibody/visitor.hpp"
#include "pinocchio/algorithm/check.hpp"
#include "pinocchio/algorithm/kinematics-derivatives.hpp"

namespace pinocchio
{
  template<typename Scalar, int Options, template<typename,int> class JointCollectionTpl, typename Matrix6xOut1, typename Matrix6xOut2>
  void
  getFrameVelocityDerivatives(const ModelTpl<Scalar,Options,JointCollectionTpl> & model,
                              DataTpl<Scalar,Options,JointCollectionTpl> & data,
                              const typename ModelTpl<Scalar,Options,JointCollectionTpl>::FrameIndex frame_id,
                              const ReferenceFrame rf,
                              const Eigen::MatrixBase<Matrix6xOut1> & v_partial_dq,
                              const Eigen::MatrixBase<Matrix6xOut2> & v_partial_dv)
  {
    typedef ModelTpl<Scalar,Options,JointCollectionTpl> Model;
    typedef DataTpl<Scalar,Options,JointCollectionTpl> Data;
    
    typedef typename Data::Matrix6x Matrix6x;
    
    EIGEN_STATIC_ASSERT_SAME_MATRIX_SIZE(Matrix6xOut1,Matrix6x);
    EIGEN_STATIC_ASSERT_SAME_MATRIX_SIZE(Matrix6xOut2,Matrix6x);
    
    PINOCCHIO_CHECK_INPUT_ARGUMENT(v_partial_dq.cols() ==  model.nv);
    PINOCCHIO_CHECK_INPUT_ARGUMENT(v_partial_dv.cols() ==  model.nv);
    assert(model.check(data) && "data is not consistent with model.");
    
    PINOCCHIO_CHECK_INPUT_ARGUMENT(frame_id <= model.frames.size(),"frame_id is larger than the number of frames");
    typedef typename Model::Frame Frame;
    typedef typename Data::Motion Motion;
    const Frame & frame = model.frames[frame_id];
    const JointIndex joint_id = frame.parent;
    
    Matrix6xOut1 & v_partial_dq_ = PINOCCHIO_EIGEN_CONST_CAST(Matrix6xOut1,v_partial_dq);
    Matrix6xOut2 & v_partial_dv_ = PINOCCHIO_EIGEN_CONST_CAST(Matrix6xOut2,v_partial_dv);
    getJointVelocityDerivatives(model,data,joint_id,rf,
                                v_partial_dq_,v_partial_dv_);
    
    // Update frame placement
    typename Data::SE3 & oMframe = data.oMf[frame_id];
    oMframe = data.oMi[joint_id] * frame.placement;
    
    typedef typename SizeDepType<1>::template ColsReturn<Matrix6xOut1>::Type ColsBlockOut1;
    typedef MotionRef<ColsBlockOut1> MotionOut1;
    typedef typename SizeDepType<1>::template ColsReturn<Matrix6xOut2>::Type ColsBlockOut2;
    typedef MotionRef<ColsBlockOut2> MotionOut2;

    Motion v_tmp;
    const typename SE3::Vector3 trans = data.oMi[joint_id].rotation() * frame.placement.translation();
    const int colRef = nv(model.joints[joint_id])+idx_v(model.joints[joint_id])-1;
    switch (rf)
    {
      case WORLD:
        // Do nothing
        break;

      case LOCAL_WORLD_ALIGNED:
        for(Eigen::DenseIndex col_id=colRef;col_id>=0;col_id=data.parents_fromRow[(size_t)col_id])
        {
          MotionOut1 m1(v_partial_dq_.col(col_id));
          m1.linear() -= trans.cross(m1.angular());
          MotionOut2 m2(v_partial_dv_.col(col_id));
          m2.linear() -= trans.cross(m2.angular());
        }
        break;

      case LOCAL:
        for(Eigen::DenseIndex col_id=colRef;col_id>=0;col_id=data.parents_fromRow[(size_t)col_id])
        {
          v_tmp = v_partial_dq_.col(col_id);
          MotionOut1(v_partial_dq_.col(col_id)) = frame.placement.actInv(v_tmp);
          v_tmp = v_partial_dv_.col(col_id);
          MotionOut2(v_partial_dv_.col(col_id)) = frame.placement.actInv(v_tmp);
        }
        break;

      default:
        break;
    }
    
  }
}

#endif // ifndef __pinocchio_algorithm_frames_derivatives_hxx__
