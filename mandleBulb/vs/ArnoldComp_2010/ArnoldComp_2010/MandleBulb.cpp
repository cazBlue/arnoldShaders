#include "ai.h"
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
 
using namespace std;
 
// UI parameters get stored into global vars
int UI_gridsize;       // sample grid resolution
int UI_max_iter;       // max Z^n+C iterations to try
float UI_power;        // exponent, the "n" in Z^n+C
float UI_spheremult;   // scales the spheres
float UI_orbitthresh;  // clears out a hollow center in the set
int UI_chunks;         // number of "chunks" for RAM management
int UI_threads;        // number of threads to use
bool UI_julia;         // mandelbrot/julia set switch
AtPoint UI_Cval;       // C value for julia sets (unused for mandelbrot)
//utility global vars
int G_counter;

 
// Calculation of inverse square root from Quake3
float inversqrt( float number )
{
        long i;
        float x2, y;
        const float threehalfs = 1.5F;
        x2 = number * 0.5F;
        y  = number;
        i  = * ( long * ) &y;                       // evil floating point bit level hacking
        i  = 0x5f3759df - ( i >> 1 );               // wtf
        y  = * ( float * ) &i;
        y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//      y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
        return y;
}
 
// Jordi Torres based on iquilez http://www.iquilezles.org/www/articles/mandelbulb/mandelbulb.htm
static AtPoint iterate(AtPoint w, float n, AtPoint C)
{
    AtPoint tp;
    float x = w.x; float x2 = x*x; float x4 = x2*x2;
    float y = w.y; float y2 = y*y; float y4 = y2*y2;
    float z = w.z; float z2 = z*z; float z4 = z2*z2;
 
    float k3 = x2 + z2;
    float k2 = inversqrt( k3*k3*k3*k3*k3*k3*k3 );
 
    float k1 = x4 + y4 + z4 - 6.0*y2*z2 - 6.0*x2*y2 + 2.0*z2*x2;
    float k4 = x2 - y2 + z2;
 
    tp.x =  64.0*x*y*z*(x2-z2)*k4*(x4-6.0*x2*z2+z4)*k1*k2;
    tp.y = -16.0*y2*k3*k4*k4 + k1*k1;
    tp.z = -8.0*y*k4*(x4*x4 - 28.0*x4*x2*z2 + 70.0*x4*z4 - 28.0*x2*z2*z4 + z4*z4)*k1*k2;
 
    return tp*n+C;
}
 
// we read the UI parameters into their global vars
static int MyInit(AtNode *mynode, void **user_ptr)
{
   *user_ptr = mynode; // make a copy of the parent procedural
   UI_gridsize = AiNodeGetInt(mynode, "gridsize");
   UI_max_iter = AiNodeGetInt(mynode, "max_iter");
   UI_power = AiNodeGetFlt(mynode, "power");
   UI_spheremult = AiNodeGetFlt(mynode, "spheremult");
   UI_orbitthresh = AiNodeGetFlt(mynode, "orbitthresh");
   UI_orbitthresh = UI_orbitthresh* UI_orbitthresh; //squared
   UI_chunks = AiNodeGetInt(mynode, "chunks");
   UI_threads = AiNodeGetInt(mynode, "threads");
   UI_julia = AiNodeGetBool(mynode, "julia");
   UI_Cval = AiNodeGetPnt(mynode, "Cval");
   G_counter = 0;
   return true;
}
 
static int MyCleanup(void *user_ptr)
{
   return true;
}
 
// we will create one node per chunk as set in the UI
static int MyNumNodes(void *user_ptr)
{
   return UI_chunks;
}
 
// this is the function that gets run on each thread
// the function (Z^n+C) is sampled at all points in a regular grid
// prisoner points with an orbit greater than UI_orbitthresh are added
// the total grid is broken into UI_chnks number of slabs on the X axis
// each slab is broken into UI_threads number of sub slabs
// and the start and end value in X is passed in and that section is sampled
void fillList(int start, int end,int chunknum,std::vector<AtPoint>& list)
{
   float inv_gridsize = 1.0f / UI_gridsize;
   // these vars are for a crude counter for percent completed
   unsigned int modder = static_cast<unsigned int>(float(end-start) / 5);
   int levelcount = 0;
   int percent = 0;
   int localcounter = 0;
 
   // only samples X in the range "start" to "end"
   for (int X = start; X < end; X++) {
      levelcount++;
      // echo out some completion info
      if (modder > 0) {
         if ((levelcount%modder) ==0 ) {
            percent += 10;
            cout <<percent << " percent of chunk " << chunknum << ",";
         }
      }
      // samples all points in Y and Z
      for (int Y = 0; Y < UI_gridsize; Y++) {
         for (int Z = 0; Z < UI_gridsize; Z++) {
            AtPoint sample;
            sample.x = (X * inv_gridsize - 0.5f) * 2.5f;
            sample.y = (Y * inv_gridsize - 0.5f) * 2.5f;
            sample.z = (Z * inv_gridsize - 0.5f) * 2.5f;
            // init the iterator
            AtPoint iterator = sample;
            // now iterate the Z^n+C function UI_max_iter number of times
            for (int iter = 0; iter < UI_max_iter; iter++) {
               if (AiV3Dot(iterator,iterator) > 4)
                  break; //orbit has left the max radius of 2....
               if (UI_julia) {
                  // each UI value of C creates a full Julia set
                  iterator = iterate(iterator,UI_power,UI_Cval);
               } else {
                  // Mandelbrot set is the centerpoints of all Julia sets
                  iterator = iterate(iterator,UI_power,sample);
               }
            }
 
            // tiny orbits are usually inside the set, disallow them.
            bool allowit = AiV3Dist2(sample,iterator) >= UI_orbitthresh;
 
            // if the orbit is inside radius 2
            // and its endpoint travelled greater than orbittthresh
            if (AiV3Dot(iterator, iterator) < 4 && allowit) {
               G_counter++; // increment global counter
               localcounter++; // increment local counter
               // this is a prisoner point, add it to the set
               list.push_back(sample);
            }
         }
      }
   }
 
   cout << "finished 1 thread of chunk " << chunknum << ",";
   cout << " num total new PTs: " << localcounter << endl;
}
 
// this builds the "points" node in Arnold and sets
// the point locations, radius, and sets it to sphere mode
static AtNode *build_node(std::vector<AtPoint>& list)
{
   AtArray *pointarr = AiArrayConvert(list.size(), 1, AI_TYPE_POINT, &list[0]);
   std::vector<AtPoint>().swap(list); // clear data used by points vector.
   AtNode *currentInstance = AiNode("points"); // initialize node
 
   AiNodeSetArray(currentInstance, "points", pointarr);
   AiNodeSetFlt(currentInstance, "radius", (2.0f/UI_gridsize) * UI_spheremult);
   AiNodeSetInt(currentInstance, "mode", 1);
   return currentInstance;
}
 
// a data structure to hold the arguments for the thread
// corresponds to the arguments to the fillList() function
struct loopArgs {
   int start;
   int end;
   int i;
   std::vector<AtPoint> list;
};
 
// a function to be passed for the thread to execute
// basically a wrapper to the fillList() function
unsigned int threadloop(void *pointer)
{
   loopArgs *mydata = (loopArgs*) pointer;
   fillList(mydata->start, mydata->end, mydata->i, mydata->list);
   return 0;
}
 
// this is the function that Arnold calls to request the nodes
// that this procedural creates.
static AtNode *MyGetNode(void *user_ptr, int i)
{
   // determine the start and end point of this chunk
   float chunksize = float(UI_gridsize) / float(UI_chunks);
   int start = static_cast<int>(i*chunksize);
   int end = static_cast<int>((i+1)*chunksize);
   if (end>UI_gridsize)
      end = UI_gridsize;
   float range = end - start;
 
   // make an array of arguments for the threads
   loopArgs *mydata;
   mydata = new loopArgs[UI_threads];
   const int MAX_THREADS = 64;
   void *threads[MAX_THREADS];
 
   // now loop through and launch the threads
   for (int tnum = 0; tnum < UI_threads; tnum++) {
      // figure out the threads start and end points for the sub-chunks
      int tstart = start + static_cast<int>((range/UI_threads)*tnum);
      int tend = start + static_cast<int>((range/UI_threads)*(tnum+1));
      mydata[tnum].start = tstart;
      mydata[tnum].end = tend;
      mydata[tnum].i = i;
      threads[tnum] = AiThreadCreate(threadloop, &mydata[tnum], 0);
   }
 
   // using AiThreadWait, wait 'til the threads finish
   size_t listlength = 0;
   for (int tnum = 0; tnum < UI_threads; tnum++) {
      AiThreadWait(threads[tnum]);
      // sum up the length of all threads lists
      listlength += mydata[tnum].list.size();
   }
 
   // a vector to hold all the point data
   std::vector<AtPoint> allpoints;
   allpoints.reserve(listlength);
 
   // concatenate all the vectors returned by the threads
   for (int tnum = 0; tnum < UI_threads; tnum++) {
      allpoints.insert(allpoints.end(), mydata[tnum].list.begin(), mydata[tnum].list.end());
      std::vector<AtPoint>().swap(mydata[tnum].list); // clear data
   }
   delete[] mydata;
   for (int k = 0; k < UI_threads; k++)
      AiThreadClose(threads[k]);
 
   cout << "total sphere count: " << G_counter << endl;
   // if it's empty, return a null and Arnold handles it well.
   // passing a node with no geometry causes errors.
   if (listlength == 0)
      return NULL;
   // build the AiNode("points")
   return build_node(allpoints);
}
// DSO hook
#ifdef __cplusplus
extern "C"
{
#endif
 
AI_EXPORT_LIB int ProcLoader(AtProcVtable *vtable)
// vtable passed in by proc_loader macro define
{
   vtable->Init     = MyInit;
   vtable->Cleanup  = MyCleanup;
   vtable->NumNodes = MyNumNodes;
   vtable->GetNode  = MyGetNode;
   strcpy(vtable->version, AI_VERSION);
   return 1;
}
 
#ifdef __cplusplus
}
#endif