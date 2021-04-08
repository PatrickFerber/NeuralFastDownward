#include <torch/script.h> // One-stop header.

#include <iostream>
#include <memory>

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cerr << "usage: example-app <path-to-exported-script-module>\n";
    return -1;
  }


  torch::jit::script::Module module;
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    module = torch::jit::load(argv[1]);
  }
  catch (const c10::Error& e) {
    std::cerr << "error loading the model\n";
    return -1;
  }

  std::cout << "ok\n";

  // Create a vector of inputs. Add multiple tensors to inputs only if the
  // network has multiple inputs. To predict for a batch of samples see second
  // section.
  std::vector<torch::jit::IValue> inputs;
  at::Tensor tensor = torch::ones({1, 3});
  auto a = tensor.accessor<float, 2>();
  std::cout << "Tensor" << tensor << std::endl;
  std::cout << "First value" << tensor[0] << std::endl;
  std::cout << "First value of first value" << tensor[0][0] << std::endl;
  a[0][1] = 2;
  std::cout << "Adapted Tensor" << tensor << std::endl;
  inputs.push_back(tensor);

  at::Tensor output = module.forward(inputs).toTensor();
  std::cout << "Output " << output << std::endl;

  std::cout << "Forward pass Batch" << std::endl;
  inputs.clear();
  std::vector<at::Tensor> samples;
  samples.push_back(torch::ones({1, 3}));
  samples.push_back(torch::ones({1, 3}));
  inputs.push_back(torch::cat(samples));
  auto raw_output = module.forward(inputs);
  auto batch_tensor = raw_output.toTensor();
  std::cout << "dim" << batch_tensor.dim() << std::endl;
  std::cout << "SIze" << batch_tensor.size(0) << std::endl;
  std::cout << "Tensor output" << batch_tensor << std::endl;
  std::cout << "Tensor output" << batch_tensor[0] << std::endl;
  std::cout << "Tensor output" << batch_tensor[1] << std::endl;
  auto b = batch_tensor.accessor<float, 1>();
  for (int64_t i = 0; i < batch_tensor.size(0); ++i){
    std::cout << "Tensor output" << i << " " << b[0] << std::endl;
  }

  // does not work. Most likely requires a network with multiple outputs
  //  std::cout << "TensorVector output" << raw_output.toTensorVector() << std::endl;

  std::cout << "YEROS" << torch::zeros({1, 3}) << std::endl;
}
