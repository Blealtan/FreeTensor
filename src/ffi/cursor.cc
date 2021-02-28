#include <cursor.h>
#include <ffi.h>

namespace ir {

void init_ffi_cursor(py::module_ &m) {
    py::class_<Cursor>(m, "Cursor")
        .def("node", &Cursor::node)
        .def("nid", &Cursor::node);
}

} // namespace ir

