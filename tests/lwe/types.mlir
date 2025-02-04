// RUN: heir-opt %s 2>&1 | FileCheck %s

// This simply tests for syntax.

#encoding = #lwe.bit_field_encoding<
  cleartext_start=14,
  cleartext_bitwidth=3>
#params = #lwe.lwe_params<cmod=7917, dimension=10>
!ciphertext = !lwe.lwe_ciphertext<encoding = #encoding, lwe_params = #params>



// CHECK-LABEL: test_valid_lwe_ciphertext
func.func @test_valid_lwe_ciphertext(%arg0 : !ciphertext) -> !ciphertext {
  return %arg0 : !ciphertext
}


!ciphertext_noparams = !lwe.lwe_ciphertext<encoding = #encoding>

// CHECK-LABEL: test_valid_lwe_ciphertext_unspecified
func.func @test_valid_lwe_ciphertext_unspecified(%arg0 : !ciphertext_noparams) -> !ciphertext_noparams {
  return %arg0 : !ciphertext_noparams
}

#rlwe_params = #lwe.rlwe_params<cmod=7917, dimension=10, polyDegree=1024>
!ciphertext_rlwe = !lwe.rlwe_ciphertext<encoding = #encoding, rlwe_params = #rlwe_params>

// CHECK-LABEL: test_valid_rlwe_ciphertext
func.func @test_valid_rlwe_ciphertext(%arg0 : !ciphertext_rlwe) -> !ciphertext_rlwe {
  return %arg0 : !ciphertext_rlwe
}
