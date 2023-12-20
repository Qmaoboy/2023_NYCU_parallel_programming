#include "PPintrin.h"

// implementation of absSerial(), but it is vectorized using PP intrinsics
void absVector(float *values, float *output, int N)
{
  __pp_vec_float x;
  __pp_vec_float result;
  __pp_vec_float zero = _pp_vset_float(0.f);
  __pp_mask maskAll, maskIsNegative, maskIsNotNegative;

  //  Note: Take a careful look at this loop indexing.  This example
  //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
  //  Why is that the case?
  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {

    // All ones
    maskAll = _pp_init_ones();

    // All zeros
    maskIsNegative = _pp_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _pp_vload_float(x, values + i, maskAll); // x = values[i];

    // Set mask according to predicate
    _pp_vlt_float(maskIsNegative, x, zero, maskAll); // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _pp_vsub_float(result, zero, x, maskIsNegative); //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _pp_mask_not(maskIsNegative); // } else {

    // Execute instruction ("else" clause)
    _pp_vload_float(result, values + i, maskIsNotNegative); //   output[i] = x; }

    // Write results back to memory
    _pp_vstore_float(output + i, result, maskAll);
  }
}

void clampedExpVector(float *values, int *exponents, float *output, int N)
{
  //
  // PP STUDENTS TODO: Implement your vectorized version of
  // clampedExpSerial() here.
  //
  // Your solution should work for any value of
  // N and VECTOR_WIDTH, not just when VECTOR_WIDTH divides N
  //

  __pp_vec_float x;
  __pp_vec_int y;
  __pp_vec_int counter;
  float clampvalue=9.999999f;
  __pp_vec_float max = _pp_vset_float(clampvalue);
  __pp_vec_float res;
  __pp_vec_int one = _pp_vset_int(1);
  __pp_vec_int zero = _pp_vset_int(0);
  __pp_mask maskAll, maskexp,maskpostive,masknegative, count, maskclamped;

  // Deal with remaing element when N % VECTOR_WIDTH != 0
  for (int i = N;i < N + VECTOR_WIDTH;i++) {
    values[i] = 0.0f;
    exponents[i] = 1;
  }

  //
  // PP STUDENTS TODO: Implement your vectorized version of
  // clampedExpSerial() here.
  //
  // Your solution should work for any value of
  // N and VECTOR_WIDTH, not just when VECTOR_WIDTH divides N
  //
  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
    // All ones
    maskAll = _pp_init_ones();

    // Init exp mask val to zeros
    maskexp = _pp_init_ones(0);
    maskclamped = _pp_init_ones(0); // init maskclamped to zeros
    counter = _pp_vset_int(0);

    _pp_vload_float(x, values + i, maskAll);         // x = values[i:i:VECTOR_WIDTH]

    _pp_vload_int(y, exponents + i, maskAll);     // exp = exponents[i:i:VECTOR_WIDTH]

    _pp_veq_int(maskpostive, y, zero, maskAll); //if (y == 0)

    _pp_vset_float(res, 1.0f, maskpostive); //   result[j] = 1.0f

    masknegative = _pp_mask_not(maskpostive);     // else

    _pp_vmove_float(res, x, masknegative); // float result = x;

    _pp_vsub_int(counter, y, one, masknegative);    //  int count = y - 1;

    _pp_vgt_int(count, counter, zero, maskAll); // maskCounterIsPositive = expCounter > 0

    while (_pp_cntbits(count)) {  // while (count > 0)
      _pp_vmult_float(res, res, x, count);        // result *= x;

      _pp_vsub_int(counter, counter, one, count);  //count--;

      _pp_vgt_int(count, counter, zero, maskAll);    // maskCounterIsPositive = expCounter > 0
    }

    _pp_vgt_float(maskclamped, res, max, maskAll); // if (result > 9.999999f)

    _pp_vset_float(res, clampvalue, maskclamped); // result = 9.999999f;

    // Write results back to memory
    _pp_vstore_float(output + i, res, maskAll); // output[i] = result;
  }
}

// returns the sum of all elements in values
// You can assume N is a multiple of VECTOR_WIDTH
// You can assume VECTOR_WIDTH is a power of 2
float arraySumVector(float *values, int N)
{

  //
  // PP STUDENTS TODO: Implement your vectorized version of arraySumSerial here
  //

  __pp_vec_float x, res;
  __pp_mask maskAll;

  float output[VECTOR_WIDTH];
  int veccount = VECTOR_WIDTH;
  maskAll = _pp_init_ones();

  // init result to 0
  res = _pp_vset_float(0.0f);

  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
    _pp_vload_float(x, values + i, maskAll);

    _pp_vadd_float(res, res, x, maskAll);
  }


  while (veccount>=2){
    _pp_hadd_float(res, res);

    _pp_interleave_float(res, res);

    veccount/=2; // power of 2 summantion
  }

  _pp_vstore_float(output, res, maskAll);

  return output[0];
}
