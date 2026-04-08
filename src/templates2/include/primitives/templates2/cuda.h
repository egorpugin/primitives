#pragma once

#include <cuda_runtime.h>

#include <iostream>

namespace primitives::cuda {

void print_gpu_info() {
    int deviceCount = 0;
    cudaError_t error = cudaGetDeviceCount(&deviceCount);

    if (error != cudaSuccess) {
        std::cerr << "cudaGetDeviceCount failed: " << cudaGetErrorString(error) << std::endl;
        return;
    }

    std::cout << "Found " << deviceCount << " CUDA device(s)" << std::endl;

    for (int device = 0; device < deviceCount; device++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, device);

        auto line = "========================================";

        std::cout << "\n" << line << std::endl;
        std::cout << "Device " << device << ": " << prop.name << std::endl;
        std::cout << line << std::endl;
        std::cout << "Compute Capability:        " << prop.major << "." << prop.minor << std::endl;
        std::cout << "Total Global Memory:       " << prop.totalGlobalMem / (1024 * 1024) << " MB" << std::endl;
        std::cout << "Multiprocessor Count:      " << prop.multiProcessorCount << std::endl;
        std::cout << "CUDA Cores (estimated):    " << prop.multiProcessorCount * 64 << " (varies by arch)" << std::endl;
        //std::cout << "Clock Rate:                " << prop.clockRate / 1000 << " MHz" << std::endl;
        std::cout << "Memory Bus Width:          " << prop.memoryBusWidth << " bits" << std::endl;
        std::cout << "L2 Cache Size:             " << prop.l2CacheSize / 1024 << " KB" << std::endl;
        std::cout << "Warp Size:                 " << prop.warpSize << std::endl;
        std::cout << "Max Threads per Block:     " << prop.maxThreadsPerBlock << std::endl;
        std::cout << "Max Grid Size:             " << prop.maxGridSize[0] << " x "
                  << prop.maxGridSize[1] << " x " << prop.maxGridSize[2] << std::endl;
        std::cout << "Integrated GPU:            " << (prop.integrated ? "Yes" : "No") << std::endl;
        std::cout << "ECC Support:               " << (prop.ECCEnabled ? "Enabled" : "Disabled") << std::endl;
        std::cout << "PCI Bus/Device/Domain:     " << prop.pciBusID << "/"
                  << prop.pciDeviceID << "/" << prop.pciDomainID << std::endl;
        std::cout << line << std::endl;
    }
}

} // namespace primitives::cuda
