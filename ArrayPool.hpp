
#ifndef __ARRAYPOOL_HPP__
#define __ARRAYPOOL_HPP__

#include "Array.hpp"
#include "PartitionPool.hpp"
#include <unordered_map>
#include <list>
#include <vector>

using namespace std;

typedef list<Array> ArrayList;
typedef unordered_map<size_t, ArrayList*> ArrayPoolMap; 

/*
 * Array Pool:
 *      m_pools:    use [Partition info, buffer_data_type] as identical key
 *                  value is an pointer of Arraylist
 */

class ArrayPool{
    private:
        ArrayPoolMap m_pools;
    
    public:
        // get an ArrayPtr from m_pools based on hash created by key:
        // [comm, process_size, (gx, gy, gz), stencil_width, buffer_data_type]
        ArrayPtr get(MPI_Comm comm, int size, vector<int> gs, int stencil_width = 1, int data_type = 0) {
            Array* ap;
            size_t par_hash = Partition::gen_hash(comm, size, gs, stencil_width);
            
            // only use array_hash = par_hash + buffer_data_type (may be a bug)
            size_t array_hash = par_hash + data_type;

            ArrayPoolMap::iterator it = m_pools.find(array_hash);
            
            //    not found in ArrayPool 
            // OR found, but arraylist is empty
            if (it == m_pools.end() || it->second.size() < 1) { 
                PartitionPtr par_ptr = PartitionPool::global()->
                    get(comm, size, gs, stencil_width, par_hash);
                ap = new Array(par_ptr, data_type);
                ap->set_hash(array_hash);
            else {
                ap = it->second->back();
                it->second->pop_back();
            }

            // ArrayPtr constructor with (Array pointer, Del del)
            return ArrayPtr(ap, 
                [](Array* arr_p) {
                    ArrayPool::global()->dispose(arr_p);
                });
        }

        // get an ArrayPtr from m_pools based on hash created by key:
        // [comm, lx, ly, lz, stencil_width, buffer_data_type]
        ArrayPtr get(MPI_Comm comm, vector<int> x, vector<int> y, vector<int> z, int stencil_width = 1, int data_type = 0) {
            Array* ap;
            size_t par_hash = Partition::gen_hash(comm, x, y, z, stencil_width);
            
            // only use array_hash = par_hash + buffer_data_type (may be a bug)
            size_t array_hash = par_hash + data_type;

            ArrayPoolMap::iterator it = m_pools.find(array_hash);
            
            //    not found in ArrayPool 
            // OR found, but arraylist is empty
            if (it == m_pools.end() || it->second.size() < 1) { 
                PartitionPtr par_ptr = PartitionPool::global()->
                    get(comm, size, gs, stencil_width, par_hash);
                ap = new Array(par_ptr, data_type);
                ap->set_hash(array_hash);
            else {
                ap = it->second->back();
                it->second->pop_back();
            }

            // ArrayPtr constructor with (Array pointer, Del del)
            return ArrayPtr(ap, 
                [](Array* arr_p) {
                    ArrayPool::global()->dispose(arr_p);
                });
        }

        void dispose(Array* ap){
            cout<<"ArrayPool dispose called!\n"<<endl;
            size_t array_hash = ap->hash();

            ArrayPoolMap::iterator it = m_pools.find(array_hash);
            if (it == m_pools.end()){
                ArrayList* al = new ArrayList();
                al->push_back(ap);
                m_pools[array_hash] = al;
            } else{
                it->second->push_back(ap);      
            }
        }
  
        // only need one Array Pool in each process, make it static 
        static ArrayPool* global(){
            static ArrayPool ap;
            return &ap;
        }
};


#endif