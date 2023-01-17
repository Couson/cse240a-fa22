//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Yifei Ning, Xingtong Yang";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
// gshare initials
uint8_t *gs_bht;
uint32_t global_hist;

//tornament initals
uint8_t *choice_table;
uint8_t *local_hist;
uint8_t *local_predictor;

typedef struct{
  uint8_t global_result;
  uint8_t lobal_result;
  uint8_t pc_lsb;
  uint8_t local_predict_index;
} tour_predict;

// custom initials
#define pcindex_custom 9
#define ghistBits_custom 17
int** perceptron;
int32_t global_hist_custom;
#define threshold (1.93*ghistBits_custom+14)
int prediction;
//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  // Gshare initials
  double tmp_length = 0;
  int length_table = 0;
  unsigned gmask_left;
  unsigned gmask_right;

  // Tournament Initials
  double tmp_ct_length = 0;
  int choice_length = 0;
  double tmp_lc_length = 0;
  int local_hist_length = 0;
  unsigned lmask_left;
  unsigned lmask_right;
  uint32_t local_hist_init = 0;
  double tmp_lh_length = 0;
  int lh_length = 0;
  double tmp_lp_length = 0;
  int lp_length = 0;
  
  //Custom
  int size_perceptron;

  switch (bpType) {
    case GSHARE:
      tmp_length = pow(2, ghistoryBits);
      length_table = (int)tmp_length;
      gs_bht = (uint8_t *) malloc(length_table);

      // Initilize branch history table
      for (int i=0; i < length_table; i++){
        gs_bht[i] = 1;
      }

      // Initialize branch history register
      gmask_left = 1 << (ghistoryBits - 1);
      gmask_right = gmask_left >> (ghistoryBits - 1);
      global_hist = gmask_left & gmask_right;
      break;
    case TOURNAMENT:
      // Initial choice table to choose weak global
      tmp_ct_length = pow(2, ghistoryBits);
      choice_length = (int)tmp_ct_length;
      choice_table = (uint8_t *) malloc(choice_length);

      for (int i = 0; i < choice_length; i++){
        choice_table[i] = 2;
      }

      //initial local history table to NOTTAKEN
      lmask_left = 1 << (lhistoryBits - 1);
      lmask_right = lmask_left >> (lhistoryBits - 1);
      local_hist_init = lmask_left & lmask_right;

      tmp_lh_length = pow(2, pcIndexBits);
      lh_length = (int)tmp_lh_length;
      local_hist = (uint8_t *) malloc(lh_length);

      for (int j = 0; j < lh_length; j++){
        local_hist[j] = 0;
      }

      // Initial local history predictor table to weakly not taken
      tmp_lp_length = pow(2, lhistoryBits);
      lp_length = (int)tmp_lp_length;
      local_predictor = (uint8_t *) malloc(lp_length);

      for (int k = 0; k < lp_length; k++){
        local_predictor[k] = 1;
      }

      // Initial global history infor
      tmp_length = pow(2, ghistoryBits);
      length_table = (int)tmp_length;
      gs_bht = (uint8_t *) malloc(length_table);

      // Initilize branch history table
      for (int i=0; i < length_table; i++){
        gs_bht[i] = 1;
      }

      // Initialize branch history register
      gmask_left = 1 << (ghistoryBits - 1);
      gmask_right = gmask_left >> (ghistoryBits - 1);
      global_hist = gmask_left & gmask_right;

      
    case CUSTOM:
      size_perceptron = pow(2, pcindex_custom); // 12
      perceptron = (int **)malloc(size_perceptron * sizeof(int *));
      for (int i = 0; i < size_perceptron; i++) {
          perceptron[i] = (int *)malloc((ghistBits_custom + 1 ) * sizeof(int));
          for (int j = 0; j <= ghistBits_custom; j++) { //13
              perceptron[i][j] = 0;
          }
      }

      global_hist_custom = 0;

      break;
    default:
      break;
  }

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Initial for gshare
  uint32_t pc_LSB = 0;
  uint32_t pc_mask;
  uint32_t bht_index = 0;

  // Initial for tournament
  uint32_t choice_result = 0;
  uint32_t local_predict_index = 0;
  tour_predict tr;

  // Initial for custom
  int index_perceptron = 0;
  int bit;
  int status;
  
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      pc_mask = (1 << ghistoryBits) - 1;
      pc_LSB = pc & pc_mask;
      bht_index = global_hist ^ pc_LSB;

      if(gs_bht[bht_index] == 0 | gs_bht[bht_index] == 1){
        return NOTTAKEN;
      }
      else{
        return TAKEN;
      }

    case TOURNAMENT:
      // Get pc LSB 
      pc_mask = (1 << pcIndexBits) - 1;
      pc_LSB = pc & pc_mask;

      // Get the predictor result from choice table
      choice_result = choice_table[global_hist];

      // Choose predictor
      if (choice_result == 2 | choice_result == 3){
        // choose global predictor
        if(gs_bht[global_hist] == 0 | gs_bht[global_hist] == 1){
          return NOTTAKEN;
        }
        else{
          return TAKEN;
        }
      }
      else{
        // Choose local predictor
        local_predict_index= local_hist[pc_LSB];

        if(local_predictor[local_predict_index] == 0 | local_predictor[local_predict_index] == 1){
          return NOTTAKEN;
        }
        else{
          return TAKEN;
        }

      }
    case CUSTOM:
    // Getting LSB of pc
      pc_mask = (1 << pcindex_custom) - 1;
      pc_LSB = pc & pc_mask;

      index_perceptron = pc_LSB ^ (global_hist_custom & pc_mask);

      prediction = perceptron[index_perceptron][ghistBits_custom];

      for(int i = 0; i < ghistBits_custom; i++){
        // get ith bits from global history register
        bit = (global_hist_custom >> i) & 1;

        // if bit = 0 status is -1 not taken
        if(bit == 0){
          prediction -= perceptron[index_perceptron][i];
        }
        else{
          prediction += perceptron[index_perceptron][i];
        }

 
      }

      // Make prediction
      if(prediction >= 0){
        return TAKEN;
      }
      else{
        return NOTTAKEN;
      }

      break;
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  uint32_t pc_LSB = 0;
  uint32_t pc_mask = 0;
  uint32_t bht_index = 0;
  uint32_t g_hist_mask = 0;

  // tornament
  uint32_t choice_result = 0;
  tour_predict tr;
  uint32_t l_hist_mask = 0;
  uint8_t global_pred = 0;
  uint8_t local_pred = 0;

  // Custom
  int pred_custom = 0;
  int index_perceptron;
  int outcome_value;
  int bit;

  switch (bpType) {
    case GSHARE:
      pc_mask = (1 << ghistoryBits) - 1;
      pc_LSB = pc & pc_mask;
      bht_index = global_hist ^ pc_LSB;
      
      // Update Branch history table
      if(outcome == TAKEN && gs_bht[bht_index] < 3){  
            gs_bht[bht_index]++;

      }
      else if(outcome == NOTTAKEN && gs_bht[bht_index] > 0){
            gs_bht[bht_index]--;

      }

      // Update global branch register
      global_hist = global_hist << 1 | outcome;
      g_hist_mask = (1 << ghistoryBits) - 1;
      global_hist = global_hist & g_hist_mask;


      //global_hist = global_hist | outcome;
      break;
      
    case TOURNAMENT:
      choice_result = choice_table[global_hist];
      pc_mask = (1 << pcIndexBits) - 1;
      pc_LSB = pc & pc_mask;
      
      global_pred = gs_bht[global_hist];
      local_pred = local_predictor[local_hist[pc_LSB]];
      // Updating choice table
      if(local_pred != outcome && global_pred == outcome){
        if(choice_table[global_hist] < 3){
          choice_table[global_hist]++;
        }
      }
      else if(local_pred == outcome && global_pred != outcome){
        if(choice_table[global_hist] > 0){
          choice_table[global_hist]--;
        }
      }

      // Updating global history table
      if(outcome == TAKEN && gs_bht[global_hist] < 3){  
        gs_bht[global_hist]++;

      }
      else if(outcome == NOTTAKEN && gs_bht[global_hist] > 0){
        gs_bht[global_hist]--;
      }
      global_hist = global_hist << 1 | outcome;
      g_hist_mask = (1 << ghistoryBits) - 1;
      global_hist = global_hist & g_hist_mask;

      // Updating local history table


      if(outcome == TAKEN && local_predictor[local_hist[pc_LSB]] < 3){  
        local_predictor[local_hist[pc_LSB]]++;

      }
      else if(outcome == NOTTAKEN && local_predictor[local_hist[pc_LSB]] > 0){
        local_predictor[local_hist[pc_LSB]]--;
      }

      local_hist[pc_LSB] = local_hist[pc_LSB] << 1 | outcome;
      l_hist_mask = (1 << lhistoryBits) - 1;
      local_hist[pc_LSB] = l_hist_mask & local_hist[pc_LSB];


      break;
    case CUSTOM:
      pc_mask = (1 << pcindex_custom) - 1;
      pc_LSB = pc & pc_mask;

      index_perceptron = pc_LSB ^ (global_hist_custom & pc_mask);

      pred_custom = make_prediction(pc);


      if(outcome != pred_custom || (prediction > -threshold && prediction < threshold)) {
        //Updating bias
        if(outcome == TAKEN){
          if(perceptron[index_perceptron][0] < threshold){
            perceptron[index_perceptron][0]++;
          }

        }
        else{
          if(perceptron[index_perceptron][0] > -threshold){
            perceptron[index_perceptron][0]--;
          }
        }
        
        //Update weight in perceptron
        pred_custom = 1;
        for(int i = 0; i < ghistBits_custom; i++){
          bit = (global_hist_custom >> i) & 1;

          if(outcome == bit){
            if(perceptron[index_perceptron][i] < threshold){
              perceptron[index_perceptron][i]++;
            }
          }
          else{
            if(perceptron[index_perceptron][i] > -1*threshold){
              perceptron[index_perceptron][i]--;
            }
          }
        }
        
      }

      // updating global history
      global_hist_custom = global_hist_custom << 1 | outcome;
      g_hist_mask = (1 << ghistBits_custom) - 1;
      global_hist_custom = global_hist_custom & g_hist_mask;

    default:
      break;
  }
}
