1. Download libtorch
2. set PATH_TORCH to point to the extracted libtorch directory
3. Build the cmake project
4. run `create_model.py` to create the model (and playing around with torch)
5. execute `run.sh`
 - The first four TestTorch Outputs should be:
   TestTorch Output: 0.856784
   TestTorch Output: 0.000999021
   TestTorch Output: -0.0933112
   TestTorch Output: 0.763473
 - every other output should be a repetition of the last one
 - Attention: The first output is a bit hidden, just use
   `run.sh | grep "TestTorch Output:" | head -n 5` 