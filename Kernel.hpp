#ifndef __KERNEL_HPP__
#define __KERNEL_HPP__

#include "NodePool.hpp"
#include "NodeDesc.hpp"
#include "Function.hpp"
#include "Internal.hpp"
#include <vector>
using namespace std;

namespace oa {
  namespace kernel {

    ///:mute
    ///:include "NodeType.fypp"
    ///:endmute
    ///:for k in [i for i in L]
    ///:set name = k[1]
    ///:set sy = k[2]
    ///:set ef = k[7]
    // return ANS = ${ef}$
    ArrayPtr kernel_${name}$(vector<ArrayPtr> &ops_ap);

    ///:endfor
    
    // ap = u {+ - * /} v
    ///:mute
    ///:include "NodeType.fypp"
    ///:endmute
    ///:for k in [i for i in L if i[3] == 'A']
    ///:set name = k[1]
    ///:set sy = k[2]
    // A = U ${sy}$ V
    template <typename T1, typename T2, typename T3>
    ArrayPtr t_kernel_${name}$(vector<ArrayPtr> &ops_ap) {
      ArrayPtr u = ops_ap[0];
      ArrayPtr v = ops_ap[1];
      ArrayPtr ap;

      int u_dt = u->get_data_type();
      int v_dt = v->get_data_type();
      int dt = oa::utils::cast_data_type(u_dt, v_dt);

      if (u->is_seqs_scalar()) {
        ap = ArrayPool::global()->get(v->get_partition(), dt);
        T2 scalar = *(T2*) u->get_buffer();
        oa::internal::const_${name}$_buffer(
          (T1 *) ap->get_buffer(),
          scalar,
          (T3 *) v->get_buffer(),
          ap->buffer_size()
        );
      } else if (v->is_seqs_scalar()) {
        ap = ArrayPool::global()->get(u->get_partition(), dt);
        T3 scalar = *(T3*) v->get_buffer();
        oa::internal::buffer_${name}$_const(
          (T1 *) ap->get_buffer(),
          (T2 *) u->get_buffer(),
          scalar,
          ap->buffer_size()
        );
      } else {
        PartitionPtr upar = u->get_partition();
        PartitionPtr vpar = v->get_partition();
        assert(upar->get_comm() == vpar->get_comm());

/*        // U and V must have same shape
        assert(oa::utils::is_equal_shape(upar->shape(), vpar->shape()));
*/
        ap = ArrayPool::global()->get(upar, dt);
        if (upar->equal(vpar)) {
          oa::internal::buffer_${name}$_buffer(
            (T1 *) ap->get_buffer(),
            (T2 *) u->get_buffer(),
            (T3 *) v->get_buffer(),
            ap->buffer_size()
          );
        } else {
          ArrayPtr tmp = oa::funcs::transfer(v, upar);
          oa::internal::buffer_${name}$_buffer(
            (T1 *) ap->get_buffer(),
            (T2 *) u->get_buffer(),
            (T3 *) tmp->get_buffer(),
            ap->buffer_size()
          );
        }
      }
      return ap;
    }

    ///:endfor
  
    ///:mute
    ///:set K = [['gt','>'], ['ge', '>='], ['lt', '<'],['le', '<='], &
                 ['eq','=='], ['ne','/='],['and','&&'],['or','||']]
    ///:endmute
    ///:for t in K
    ///:set name = t[0]
    ///:set sy = t[1]
    // A = U ${sy}$ V
    template <typename T1, typename T2, typename T3>
    ArrayPtr t_kernel_${name}$(vector<ArrayPtr> &ops_ap) {
      ArrayPtr u = ops_ap[0];
      ArrayPtr v = ops_ap[1];
      ArrayPtr ap;

      int u_dt = u->get_data_type();
      int v_dt = v->get_data_type();
      int dt = 0;

      if (u->is_seqs_scalar()) {
        ap = ArrayPool::global()->get(v->get_partition(), dt);
        T2 scalar = *(T2*) u->get_buffer();
        oa::internal::const_${name}$_buffer(
          (T1 *) ap->get_buffer(),
          scalar,
          (T3 *) v->get_buffer(),
          ap->buffer_size()
        );
      } else if (v->is_seqs_scalar()) {
        ap = ArrayPool::global()->get(u->get_partition(), dt);
        T3 scalar = *(T3*) v->get_buffer();
        oa::internal::buffer_${name}$_const(
          (T1 *) ap->get_buffer(),
          (T2 *) u->get_buffer(),
          scalar,
          ap->buffer_size()
        );
      } else {
        PartitionPtr upar = u->get_partition();
        PartitionPtr vpar = v->get_partition();
        assert(upar->get_comm() == vpar->get_comm());

/*        // U and V must have same shape
        assert(oa::utils::is_equal_shape(upar->shape(), vpar->shape()));
*/
        ap = ArrayPool::global()->get(upar, dt);
        if (upar->equal(vpar)) {
          oa::internal::buffer_${name}$_buffer(
            (T1 *) ap->get_buffer(),
            (T2 *) u->get_buffer(),
            (T3 *) v->get_buffer(),
            ap->buffer_size()
          );
        } else {
          ArrayPtr tmp = oa::funcs::transfer(v, upar);
          oa::internal::buffer_${name}$_buffer(
            (T1 *) ap->get_buffer(),
            (T2 *) u->get_buffer(),
            (T3 *) tmp->get_buffer(),
            ap->buffer_size()
          );
        }
      }
      return ap;
    }

    ///:endfor

    // ap = {max/min}u 
    ///:mute
    ///:include "NodeType.fypp"
    ///:endmute
    ///:for k in [i for i in L if i[3] == 'E']
    ///:set name = k[1]
    ///:set sy = k[2]
    // A = ${sy}$ U
    template<typename T>
    ArrayPtr t_kernel_${name}$(vector<ArrayPtr> &ops_ap) {
      ArrayPtr u = ops_ap[0];
      int u_dt = u->get_data_type();
      int dt = u_dt;
      int sw = u->get_partition()->get_stencil_width();

      // part_val, total_val;
      struct {
          T value;
          int thread_id;
          int x;
          int y;
          int z;
      } local, global;

      int3 pos = oa::internal::buffer_${name}$_const(
        local.value, 
        (T*) u->get_buffer(),
        u->get_local_box(),
        sw,
        u->buffer_size()
      );

      MPI_Comm comm = u->get_partition()->get_comm();
      int rankID = oa::utils::get_rank(comm);

      local.x = pos[0];
      local.y = pos[1];
      local.z = pos[2];
      local.thread_id = rankID;

      MPI_Reduce(&local, &global, 1, MPI_DOUBLE_INT, MPI_${sy}$LOC, 0, comm);

      int answer_thread;
      if (rankID == 0) {
        std::cout<<"the root received:"<<global.value<<" @"<<global.thread_id<<std::endl;
        answer_thread = global.thread_id;
      }

      MPI_Bcast(&answer_thread, 1, MPI_INT, 0, comm);

      if (rankID == answer_thread) {
        std::cout<<"\nanswer is on rank"<<rankID<<",my value="<<local.value<<" and local postion=["<<local.x<<","<<local.y<<","<<local.z<<"]"<<std::endl;
        global.value = local.value;
        global.x = local.x;
        global.y = local.y;
        global.z = local.z;
      }

      MPI_Bcast(&global.value, 1, MPI_INT, answer_thread, comm);
      MPI_Bcast(&global.x, 1, MPI_INT, answer_thread, comm);
      MPI_Bcast(&global.y, 1, MPI_INT, answer_thread, comm);
      MPI_Bcast(&global.z, 1, MPI_INT, answer_thread, comm);

      
      std::cout<<"\nfinish by rank"<<rankID<<"my value="<<local.value<<" and local postion=["<<local.x<<","<<local.y<<","<<local.z<<"]"<<std::endl;

      ArrayPtr ap = oa::funcs::get_seq_scalar(global.value);
      return ap;

    }

    ///:endfor

    // salar = sum_scalar(A) 
    template <typename T>
    ArrayPtr t_kernel_sum_scalar(vector<ArrayPtr> &ops_ap) {
      ArrayPtr u = ops_ap[0];
      int u_dt = u->get_data_type();
      int sw = u->get_partition()->get_stencil_width();

      MPI_Comm comm = u->get_partition()->get_comm();
      int rankID = oa::utils::get_rank(comm);
      int mpisize =  oa::utils::get_size(comm);

      T temp1,temp2;
      T *local_sum = &temp1;
      T *all_sum = &temp2;
      oa::internal::buffer_sum_scalar_const(
          (T*) local_sum, 
          (T*) u->get_buffer(),
          u->get_local_box(),
          sw,
          u->buffer_size()
          );

      //std::cout<<"mpi"<<rankID<<" local_sum ="<<*local_sum<<std::endl;
      MPI_Datatype mpidt = oa::utils::mpi_datatype(u_dt);
      MPI_Allreduce(local_sum, all_sum, 1, mpidt, MPI_SUM, comm);
      ArrayPtr ap = oa::funcs::get_seq_scalar(*all_sum);
      //std::cout << "The sum is: " << *all_sum << std::endl;
      return ap;
    }

    //sum to x 
    template <typename T>
    ArrayPtr t_kernel_csum_x(vector<ArrayPtr> &ops_ap) {
      ArrayPtr ap;
      ArrayPtr u = ops_ap[0];
      int u_dt = u->get_data_type();
      int sw = u->get_partition()->get_stencil_width();

      MPI_Comm comm = u->get_partition()->get_comm();
      int rankID = oa::utils::get_rank(comm);
      int mpisize = oa::utils::get_size(comm);
      MPI_Datatype mpidt = oa::utils::mpi_datatype(u_dt);

      PartitionPtr upar = u->get_partition();
      ap = ArrayPool::global()->get(upar, u_dt);
      Shape sp = upar->procs_shape();

      vector<int> vi = upar->get_procs_3d(rankID);

      int xs, xe, ys, ye, zs, ze;
      u->get_local_box().get_corners(xs, xe, ys, ye, zs, ze, sw);
      int buffersize = (ye-ys-2*sw)*(ze-zs-2*sw);
      T * buffer = new T[buffersize];

      for(int i = sp[0]-1; i >= 0; i--)
      {
        int type;  //type:   top 2  mid 1  bottom 0
        if(i == sp[0]-1) 
          type = 2;
        else if(i == 0) 
          type = 0;
        else
          type = 1;

        for(int j = 0; j < sp[1]; j++)
          for(int k = 0; k < sp[2]; k++){
            int sendid = upar->get_procs_rank(i, j, k);
            int receid = -1;
            if(i != 0)
              receid = upar->get_procs_rank(i-1, j, k);
            if(rankID == sendid){
              oa::internal::buffer_csum_x_const(
                  (T*) ap->get_buffer(),
                  (T*) u->get_buffer(),
                  u->get_local_box(),
                  sw,
                  u->buffer_size(),
                  buffer,
                  type
                  );

              if(i != 0)
                MPI_Send(buffer, buffersize, mpidt, receid, 0, comm);
            }
            if(rankID == receid)
              MPI_Recv(buffer, buffersize, mpidt, sendid, 0, comm, MPI_STATUS_IGNORE);

          }
        MPI_Barrier(comm);
      }

      delete []buffer;
      return ap;
    }

    //csum to y
    template <typename T>
    ArrayPtr t_kernel_csum_y(vector<ArrayPtr> &ops_ap) {
      ArrayPtr ap;
      ArrayPtr u = ops_ap[0];
      int u_dt = u->get_data_type();
      int sw = u->get_partition()->get_stencil_width();

      MPI_Comm comm = u->get_partition()->get_comm();
      int rankID = oa::utils::get_rank(comm);
      int mpisize = oa::utils::get_size(comm);
      MPI_Datatype mpidt = oa::utils::mpi_datatype(u_dt);

      PartitionPtr upar = u->get_partition();
      ap = ArrayPool::global()->get(upar, u_dt);
      Shape sp = upar->procs_shape();

      vector<int> vi = upar->get_procs_3d(rankID);

      int xs, xe, ys, ye, zs, ze;
      u->get_local_box().get_corners(xs, xe, ys, ye, zs, ze, sw);
      int buffersize = (xe-xs-2*sw)*(ze-zs-2*sw);
      T * buffer = new T[buffersize];

      for(int j = sp[1]-1; j >= 0; j--)
      {
        int type;  //type:   top 2  mid 1  bottom 0
        if(j == sp[1]-1) 
          type = 2;
        else if(j == 0) 
          type = 0;
        else
          type = 1;

        for(int i = 0; i < sp[0]; i++)
          for(int k = 0; k < sp[2]; k++){
            int sendid = upar->get_procs_rank(i, j, k);
            int receid = -1;
            if(j != 0)
              receid = upar->get_procs_rank(i, j-1, k);
            if(rankID == sendid){
              oa::internal::buffer_csum_y_const(
                  (T*) ap->get_buffer(),
                  (T*) u->get_buffer(),
                  u->get_local_box(),
                  sw,
                  u->buffer_size(),
                  buffer,
                  type
                  );

              if(j != 0)
                MPI_Send(buffer, buffersize, mpidt, receid, 0, comm);
            }
            if(rankID == receid)
              MPI_Recv(buffer, buffersize, mpidt, sendid, 0, comm, MPI_STATUS_IGNORE);

          }
        MPI_Barrier(comm);
      }

      delete []buffer;
      return ap;
    }


    //csum to z
    template <typename T>
    ArrayPtr t_kernel_csum_z(vector<ArrayPtr> &ops_ap) {
      ArrayPtr ap;
      ArrayPtr u = ops_ap[0];
      int u_dt = u->get_data_type();
      int sw = u->get_partition()->get_stencil_width();

      MPI_Comm comm = u->get_partition()->get_comm();
      int rankID = oa::utils::get_rank(comm);
      int mpisize = oa::utils::get_size(comm);
      MPI_Datatype mpidt = oa::utils::mpi_datatype(u_dt);

      PartitionPtr upar = u->get_partition();
      ap = ArrayPool::global()->get(upar, u_dt);
      Shape sp = upar->procs_shape();

      vector<int> vi = upar->get_procs_3d(rankID);

      int xs, xe, ys, ye, zs, ze;
      u->get_local_box().get_corners(xs, xe, ys, ye, zs, ze, sw);
      int buffersize = (xe-xs-2*sw)*(ye-ys-2*sw);
      T * buffer = new T[buffersize];

      for(int k = sp[2]-1; k >= 0; k--)
      {
        int type;  //type:   top 2  mid 1  bottom 0
        if(k == sp[2]-1) 
          type = 2;
        else if(k == 0) 
          type = 0;
        else
          type = 1;

        for(int i = 0; i < sp[0]; i++)
          for(int j = 0; j < sp[1]; j++){
            int sendid = upar->get_procs_rank(i, j, k);
            int receid = -1;
            if(k != 0)
              receid = upar->get_procs_rank(i, j, k-1);
            if(rankID == sendid){
              oa::internal::buffer_csum_z_const(
                  (T*) ap->get_buffer(),
                  (T*) u->get_buffer(),
                  u->get_local_box(),
                  sw,
                  u->buffer_size(),
                  buffer,
                  type
                  );

              if(k != 0)
                MPI_Send(buffer, buffersize, mpidt, receid, 0, comm);
            }
            if(rankID == receid)
              MPI_Recv(buffer, buffersize, mpidt, sendid, 0, comm, MPI_STATUS_IGNORE);

          }
        MPI_Barrier(comm);
      }

      delete []buffer;
      return ap;
    }
  }
}
#endif
