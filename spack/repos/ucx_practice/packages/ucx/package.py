# Copyright 2022 range3 ( https://github.com/range3/ )
# Copyright 2013-2022 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack.pkg.builtin.ucx import Ucx as BuiltinUcx

class Ucx(BuiltinUcx):
    variant("dev", default=False, description="Use UCX Developer Builds")

    @property
    def configure_abs_path(self):
        # Absolute path to configure
        if '+dev' in self.spec:
            configure_abs_path = "contrib/configure-devel"
        else:
            configure_abs_path = "contrib/configure-release"
        return configure_abs_path
