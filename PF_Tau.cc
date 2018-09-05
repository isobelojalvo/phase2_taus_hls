#include "PF_Tau.hpp"

void file_read_in(
		track_t central_tracks[N_TRACKS],  // Number of tracks
		cluster_t central_clusters[N_CLUSTERS],  // Number of Clusters
		algo_config_t algo_config,           // algorithm configuration
		algo_outputs_t & algo_outputs        // algorithm outputs
		)
{
#pragma HLS ARRAY_PARTITION variable=central_tracks complete dim=1

	ap_uint<14> et_total_tmp = 0;

#pragma HLS PIPELINE II=8

	track_t three_prong_tau_cand[3];

	////////////////////Three Prong Tau Algorithm////////////////////
	pftau_t tau_cands[12];

	// Tau_alg Takes in charged cands, a three-prong tau candidate and the algorithm configuration 
	tau_three_prong_alg( central_tracks, three_prong_tau_cand, algo_config);

	////////////////////    Algorithm Outputs    ////////////////////
	//algo_outputs.taus = tau_cands;


}

/*
 * This creates one tau per id_eta x id_phi
 * TODO: 1.) Implememnt a 'sliding window' so that we can have some overlap between regions ()
 * 2.) Implement a data format that assumes the geometry will use 17 segments for eta, 17 segments for phi and the internal geometry is given via a different word
 *    -> Also check this proposed method against the cluster track linker
 * 3.) Implement Ales's (Amin's) sorter!
 */

void tau_three_prong_alg(track_t central_tracks[N_TRACKS], track_t three_prong_tau_cand[3], algo_config_t algo_config){
#pragma HLS PIPELINE II=8
#pragma HLS ARRAY_PARTITION variable=central_tracks complete dim=0
#pragma HLS ARRAY_PARTITION variable=three_prong_tau_cand complete dim=0

  //These are both look up tables for the detector geometry
  uint8_t detector_grid_eta[11] = {1,2,4,5,6,7,8,9,10,11,12};
  uint8_t detector_grid_phi[11] = {1,2,4,5,6,7,8,9,10,11,12};
  //We can hold the 'fine-level' 3 prong cluster variable here
  uint8_t tau_grid_pt[11][11] = {0};
  uint8_t tau_grid_eta[11][11] = {0};
  uint8_t tau_grid_phi[11][11] = {0};
  uint8_t tau_grid_nprongs[11][11] = {0};

#pragma HLS ARRAY_PARTITION variable=detector_grid_eta complete dim=0
#pragma HLS ARRAY_PARTITION variable=detector_grid_phi complete dim=0
#pragma HLS ARRAY_PARTITION variable=tau_grid_pt complete dim=0
#pragma HLS ARRAY_PARTITION variable=tau_grid_eta complete dim=0
#pragma HLS ARRAY_PARTITION variable=tau_grid_phi complete dim=0


  for (int idx = 0; idx < N_TRACKS; idx++)//note, tracks are already sorted by PT
    {
 #pragma HLS UNROLL

      //int n_found_tracks = 0;
      track_t seedtrack = central_tracks[idx];

      if(seedtrack.et < algo_config.three_prong_seed)
        continue;
      //using eta = 5, phi = 5 keeps this to about 1 minute synthesis
      //using eta = 11, phi = 11 makes this take much longer
      for(int id_eta = 0; id_eta < 5; id_eta++){
#pragma HLS UNROLL
        for(int id_phi = 0; id_phi < 5; id_phi++){
#pragma HLS UNROLL
          if(seedtrack.eta == detector_grid_eta[id_eta] && seedtrack.phi == detector_grid_phi[id_phi]){
            if(tau_grid_nprongs[id_eta][id_phi] == 2 &&
               //This OR statement is needed in case grid_eta - seedtrack_eta is negative.
               ((tau_grid_eta[id_eta][id_phi] - seedtrack.eta )<2 || (  seedtrack.eta - tau_grid_eta[id_eta][id_phi] <2))){
              tau_grid_pt[id_eta][id_phi] += seedtrack.et;
              tau_grid_nprongs[id_eta][id_phi] = 3;
            }
            else if(tau_grid_nprongs[id_eta][id_phi] == 1 &&
                    ((tau_grid_eta[id_eta][id_phi] - seedtrack.eta )<2 || (  seedtrack.eta - tau_grid_eta[id_eta][id_phi] <2))){
              tau_grid_pt[id_eta][id_phi] += seedtrack.et;
              tau_grid_nprongs[id_eta][id_phi] = 2;
            }
            else if(tau_grid_nprongs[id_eta][id_phi] == 0){
              tau_grid_pt[id_eta][id_phi] += seedtrack.et;
              tau_grid_eta[id_eta][id_phi] = seedtrack.eta;
              tau_grid_phi[id_eta][id_phi] = seedtrack.phi;
              tau_grid_nprongs[id_eta][id_phi] = 1;
            }
          }
        }
      }
    }

  //this is included simply so we 'touch' a few of the arrays and
  if(tau_grid_pt[0][0]>tau_grid_pt[1][1] && tau_grid_pt[0][0] > tau_grid_pt[2][2] && tau_grid_pt[0][0] >tau_grid_pt[3][3]){
    three_prong_tau_cand[0].et = tau_grid_pt[0][0];
    three_prong_tau_cand[0].eta = tau_grid_eta[0][0];
    three_prong_tau_cand[0].phi = tau_grid_phi[0][0];
  }



  //sorter for '1 element', takes a long time to compile
  /*
  for(int id_eta = 0; id_eta < 5; id_eta++){
#pragma HLS UNROLL
    for(int id_phi = 0; id_phi < 5; id_phi++){
#pragma HLS UNROLL
      if(tau_grid_pt[id_eta][id_phi]>three_prong_tau_cand[0].et){
        three_prong_tau_cand[0].et  = tau_grid_pt[id_eta][id_phi];
        three_prong_tau_cand[0].eta = tau_grid_eta[id_eta][id_phi];
        three_prong_tau_cand[0].phi = tau_grid_phi[id_eta][id_phi];
      }
    }
  }
  */
}


