# Temporary RV32I subset of run-tests-cases.sh; uncomment cases as support lands.
# Runtime cases still require program startup, data emission, and other backend operations.
# Explicit i64 and x86 register/syscall cases also need portable fixtures or target support.

# SRC=t1 && EXP=58 && RUN # todo: RV32I runtime backend incomplete
# SRC=t2 && EXP=1 && RUN # todo: RV32I runtime backend incomplete
# SRC=t3 && EXP=7 && RUN # todo: RV32I runtime backend incomplete
# SRC=t4 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t5 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t6 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t7 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t8 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t9 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t10 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t11 && EXP=14 && RUN # todo: RV32I runtime backend incomplete
# SRC=t12 && EXP=24 && RUN # todo: RV32I runtime backend incomplete
# SRC=t13 && EXP=2 && RUN # todo: RV32I runtime backend incomplete
# SRC=t14 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t15 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t16 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t17 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t18 && DIFF # todo: x86 registers/syscalls; backend incomplete
#SRC=t19 && EXP=120 && RUN
# SRC=t20 && EXP=7 && RUN # todo: explicit i64; backend incomplete
# SRC=t21 && EXP=12 && RUN # todo: explicit i64; backend incomplete
# SRC=t22 && EXP=16 && RUN # todo: explicit i64; backend incomplete
# SRC=t23 && EXP=17 && RUN # todo: explicit i64; backend incomplete
# SRC=t24 && EXP=7 && RUN # todo: explicit i64; backend incomplete
# SRC=t25 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t26 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t27 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t28 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t29 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t30 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t31 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t32 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t33 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t34 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t35 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t36 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
# SRC=t37 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t38 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t39 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t40 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t41 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t42 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t43 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t44 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t45 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t46 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t47 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t48 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t49 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t50 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t51 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t52 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t53 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t54 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t55 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t56 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t57 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t58 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t59 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t60 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t61 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t62 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t63 && DIFFPY # todo: x86 registers/syscalls; backend incomplete
# SRC=t64 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t65 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t66 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t67 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t68 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t69 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t70 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t71 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t72 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t73 && EXP=5 && RUN # todo: RV32I runtime backend incomplete
# SRC=t74 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t75 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t76 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t77 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t78 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t79 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t80 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t81 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t82 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t83 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t84 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t85 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t86 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t87 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
# SRC=t88 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
# SRC=t89 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
# SRC=t90 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t91 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t92 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t93 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
# SRC=t94 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t95 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t96 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t97 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
# SRC=t98 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t99 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
# SRC=t100 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t101 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t102 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t103 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t104 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t105 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t106 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t107 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t108 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t109 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t110 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t111 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t112 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t113 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t114 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t115 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t116 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t117 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t118 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t119 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t120 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t121 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t123 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t124 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t125 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t126 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t127 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t128 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t129 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t130 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t131 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t132 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t133 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t134 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t135 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t136 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t137 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t138 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t139 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t140 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t141 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t142 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t143 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t144 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t145 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t146 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t147 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t148 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t149 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t150 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t151 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t152 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t153 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t154 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t155 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t156 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t157 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t161 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t162 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t163 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t164 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t165 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t166 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t167 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t168 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t169 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t170 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t171 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t172 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t173 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t174 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t175 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t176 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t177 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t178 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t179 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t180 && COMPERR
# SRC=t181 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t182 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t183 && COMPERR
SRC=t184 && COMPERR
# SRC=t185 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t186 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t188 && COMPERR
SRC=t189 && COMPERR
# SRC=t190 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t191 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t192 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t193 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t194 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t195 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t196 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t197 && COMPERR
# SRC=t198 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t199 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t200 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t201 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t202 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t203 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t204 && COMPERR
# SRC=t205 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t206 && COMPERR
# SRC=t207 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t208 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t210 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t211 && COMPERR
SRC=t212 && COMPERR
SRC=t213 && COMPERR
SRC=t214 && COMPERR
SRC=t215 && COMPERR
SRC=t216 && COMPERR
# SRC=t217 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t218 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t219 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t220 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t221 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t222 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t223 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t224 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t225 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t226 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t227 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t228 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t229 && COMPERR
SRC=t230 && COMPERR
SRC=t231 && COMPERR
SRC=t232 && COMPERR
# SRC=t233 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t234 && COMPERR
SRC=t235 && COMPERR
SRC=t236 && COMPERR
SRC=t237 && COMPERR
SRC=t238 && COMPERR
# SRC=t239 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t240 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t241 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t242 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t243 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t245 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t246 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t249 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t248 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t250 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t251 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t252 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t253 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t254 && COMPERR
# SRC=t255 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t256 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t257 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t258 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t259 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t260 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t261 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t262 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t263 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t265 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t266 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t267 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t269 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t270 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t271 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t272 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t273 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t275 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t277 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t278 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t279 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t299 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t280 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t286 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t287 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t289 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t290 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t291 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t292 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t293 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t294 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t295 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t296 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t298 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t300 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t301 && COMPERR
# SRC=t302 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t303 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t304 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t307 && COMPERR
# SRC=t308 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t311 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t312 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t313 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t314 && COMPERR
SRC=t315 && COMPERR
# SRC=t316 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t317 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t318 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t319 && COMPERR
# SRC=t320 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t321 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t322 && COMPERR
SRC=t323 && COMPERR
# SRC=t324 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t325 && EXP=0 && RUN_NO_CHECKS # todo: explicit i64; backend incomplete
# SRC=t326 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t327 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t328 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t329 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t330 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t331 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t332 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t333 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t334 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t335 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t336 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t337 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t338 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t339 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t340 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t341 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t342 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t343 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t344 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t345 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t346 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t347 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t348 && EXP=255 && RUN_ERR_OPTS "--vars=65536 --checks=lower,line" # todo: RV32I runtime backend incomplete
# SRC=t349 && EXP=255 && RUN_ERR_OPTS "--vars=65536 --checks=upper,line" # todo: RV32I runtime backend incomplete
# SRC=t350 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t351 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t352 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t353 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t355 && EXP=255 && RUN_ERR # todo: explicit i64; backend incomplete
# SRC=t356 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t357 && EXP=255 && RUN_ERR_OPTS "--vars=65536 --checks=lower" # todo: RV32I runtime backend incomplete
# SRC=t358 && EXP=255 && RUN_ERR_OPTS "--vars=65536 --checks=upper" # todo: RV32I runtime backend incomplete
# SRC=t359 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t360 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t361 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t362 && EXP=255 && RUN_ERR # todo: explicit i64; backend incomplete
# SRC=t363 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t364 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t367 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t368 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t369 && EXP=255 && RUN_ERR_OPTS "--vars=65536 --checks=upper,lower" # todo: RV32I runtime backend incomplete
# SRC=t370 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t371 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t372 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t373 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t374 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t375 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t376 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t377 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t378 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t379 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t380 && EXP=255 && RUN_ERR # todo: explicit i64; backend incomplete
# SRC=t381 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t382 && COMPERR
SRC=t383 && COMPERR
# SRC=t384 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t385 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t386 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t387 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t388 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t389 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t390 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t391 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t392 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t393 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t394 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t395 && COMPERR
# SRC=t396 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t397 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t398 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t399 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t400 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t401 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t402 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t403 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t404 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t405 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t406 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t407 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t408 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t409 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t410 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t411 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t412 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t413 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t414 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t415 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t416 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t417 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t421 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t422 && COMPERR
# SRC=t423 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t427 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t428 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t429 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t430 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t431 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t432 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t433 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t434 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t435 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t436 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t437 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t438 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t439 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t440 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t441 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t442 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t443 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t444 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t445 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t446 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t447 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t448 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t449 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t450 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t451 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t452 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t453 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t454 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t455 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t456 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t457 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t458 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t459 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t460 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t461 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t462 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t463 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t464 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t465 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t466 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t466 && EXP=0 && OPTS="--vars=262144 --checks=frame --reproduce-source" RUN # todo: explicit i64; backend incomplete
# SRC=t466 && EXP=255 && OPTS="--vars=64 --checks=frame --reproduce-source" RUN # todo: explicit i64; backend incomplete
# SRC=t467 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t467 && EXP=0 && OPTS="--vars=262144 --checks=frame --reproduce-source" RUN # todo: explicit i64; backend incomplete
# SRC=t468 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t469 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t470 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t471 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t472 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t473 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t474 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t475 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t476 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t477 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t478 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t479 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t480 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t481 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t482 && EXP=42 && RUN # todo: RV32I runtime backend incomplete
# SRC=t483 && EXP=42 && RUN # todo: RV32I runtime backend incomplete
# SRC=t484 && EXP=42 && RUN # todo: RV32I runtime backend incomplete
SRC=t485 && COMPERR
