#include "outline_provider_factory.h"
#include "python_outline_provider.h"
#include "cpp_outline_provider.h"

namespace SprintIDE {

    std::unique_ptr<IOutlineProvider>
    OutlineProviderFactory::forLanguage(const QString& languageId)
    {
        const QString id = languageId.toLower();
        if (id == "python")
        return std::make_unique<PythonOutlineProvider>();
        if (id == "cpp" || id == "c" || id == "cxx" || id == "cc"
            || id == "h"   || id == "hpp" || id == "hh" || id == "hxx"
            || id == "qt") {
            return std::make_unique<CppOutlineProvider>();
        }
        return nullptr;
    }

} // namespace SprintIDE