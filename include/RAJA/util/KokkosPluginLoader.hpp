#ifndef RAJA_Kokkos_Plugin_Loader_HPP
#define RAJA_Kokkos_Plugin_Loader_HPP

#include <memory>
#include <vector>

#include "RAJA/util/PluginOptions.hpp"
#include "RAJA/util/PluginStrategy.hpp"

namespace RAJA {
namespace util {

  class KokkosPluginLoader : public ::RAJA::util::PluginStrategy
  {
    using Parent = ::RAJA::util::PluginStrategy;
    typedef void (*init_function)(const int, const uint64_t, const uint32_t, void*);
    typedef void (*pre_function)(const char*, const uint32_t, uint64_t*);
    typedef void (*post_function)(uint64_t);
    typedef void (*finalize_function)();

  public:
    KokkosPluginLoader();

    void init(RAJA::util::PluginOptions p);

    void preLaunch(RAJA::util::PluginContext& p);

    void postLaunch(RAJA::util::PluginContext& p);

    void finalize();

  private:
    void initPlugin(const std::string &path);
    
    void initDirectory(const std::string &path);

    std::vector<init_function> init_functions;
    std::vector<pre_function> pre_functions;
    std::vector<post_function> post_functions;
    std::vector<finalize_function> finalize_functions;

  };  // end KokkosPluginLoader class

  void linkKokkosPluginLoader();

}  // end namespace util
}  // end namespace RAJA

#endif
