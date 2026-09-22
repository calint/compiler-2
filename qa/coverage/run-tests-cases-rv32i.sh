# Temporary RV32I subset of run-tests-cases.sh; uncomment cases as support lands.
# Runtime cases are promoted from RUN_NO_CHECKS as checked paths are validated.
# Explicit i64 and x86 register/syscall cases also need portable fixtures or target support.

# SRC=t1 && EXP=58 && RUN # todo: RV32I runtime backend incomplete
SRC=t2 && EXP=1 && RUN_NO_CHECKS
SRC=t3 && EXP=7 && RUN_NO_CHECKS
SRC=t4 && EXP=0 && RUN_NO_CHECKS
SRC=t5 && EXP=0 && RUN_NO_CHECKS
SRC=t6 && EXP=0 && RUN_NO_CHECKS
SRC=t7 && EXP=0 && RUN_NO_CHECKS
SRC=t8 && EXP=0 && RUN_NO_CHECKS
SRC=t9 && EXP=0 && RUN_NO_CHECKS
SRC=t10 && EXP=0 && RUN_NO_CHECKS
SRC=t11 && EXP=14 && RUN_NO_CHECKS
SRC=t12 && EXP=24 && RUN_NO_CHECKS
SRC=t13 && EXP=2 && RUN_NO_CHECKS
SRC=t14 && EXP=0 && RUN_NO_CHECKS
# SRC=t15 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t16 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t17 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t18 && DIFF # todo: x86 registers/syscalls; backend incomplete
#SRC=t19 && EXP=120 && RUN
SRC=t20 && EXP=7 && RUN_NO_CHECKS
SRC=t21 && EXP=12 && RUN_NO_CHECKS
SRC=t22 && EXP=16 && RUN_NO_CHECKS
SRC=t23 && EXP=17 && RUN_NO_CHECKS
SRC=t24 && EXP=7 && RUN_NO_CHECKS
# SRC=t25 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t26 && EXP=0 && RUN_NO_CHECKS
SRC=t27 && EXP=0 && RUN_NO_CHECKS
# SRC=t28 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t29 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t30 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
SRC=t31 && EXP=0 && RUN_NO_CHECKS
SRC=t32 && EXP=0 && RUN_NO_CHECKS
# SRC=t33 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t34 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t35 && EXP=0 && RUN_NO_CHECKS
# SRC=t36 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
# SRC=t37 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t38 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t39 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t40 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t41 && EXP=0 && RUN_NO_CHECKS
# SRC=t42 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t43 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t44 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t45 && EXP=0 && RUN_NO_CHECKS
SRC=t46 && EXP=0 && RUN_NO_CHECKS
SRC=t47 && EXP=0 && RUN_NO_CHECKS
# SRC=t48 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t49 && EXP=0 && RUN_NO_CHECKS
SRC=t50 && EXP=0 && RUN_NO_CHECKS
SRC=t51 && EXP=0 && RUN_NO_CHECKS
SRC=t52 && EXP=0 && RUN_NO_CHECKS
SRC=t53 && EXP=0 && RUN_NO_CHECKS
# SRC=t54 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t55 && EXP=0 && RUN_NO_CHECKS
# SRC=t56 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t57 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t58 && EXP=0 && RUN_NO_CHECKS
SRC=t59 && EXP=0 && RUN_NO_CHECKS
# SRC=t60 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t61 && EXP=0 && RUN_NO_CHECKS
SRC=t62 && EXP=0 && RUN_NO_CHECKS
# SRC=t63 && DIFFPY # todo: x86 registers/syscalls; backend incomplete
SRC=t64 && EXP=0 && RUN_NO_CHECKS
SRC=t65 && EXP=0 && RUN_NO_CHECKS
# SRC=t66 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t67 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t68 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t69 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t70 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t71 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t72 && EXP=0 && RUN_NO_CHECKS
SRC=t73 && EXP=5 && RUN_NO_CHECKS
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
SRC=t94 && EXP=0 && RUN_NO_CHECKS
SRC=t95 && EXP=0 && RUN_NO_CHECKS
# SRC=t96 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t97 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
# SRC=t98 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t99 && DIFFINP2 # todo: x86 registers/syscalls; backend incomplete
SRC=t100 && COMPERR
SRC=t101 && COMPERR
SRC=t102 && COMPERR
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
SRC=t117 && EXP=0 && RUN_NO_CHECKS
SRC=t118 && EXP=0 && RUN_NO_CHECKS
# SRC=t119 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t120 && EXP=255 && RUN_ERR
SRC=t121 && EXP=255 && RUN_ERR
SRC=t123 && COMPERR
SRC=t124 && COMPERR
SRC=t125 && COMPERR
SRC=t126 && COMPERR
SRC=t127 && COMPERR
# SRC=t128 && EXP=0 && RUN # todo: x86 registers/syscalls; backend incomplete
# SRC=t129 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t130 && EXP=0 && RUN
SRC=t131 && EXP=0 && RUN
SRC=t132 && EXP=0 && RUN
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
SRC=t144 && EXP=0 && RUN_NO_CHECKS
SRC=t145 && EXP=0 && RUN_NO_CHECKS
# SRC=t146 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t147 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t148 && EXP=0 && RUN_NO_CHECKS
# SRC=t149 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t150 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t151 && EXP=0 && RUN_NO_CHECKS
# SRC=t152 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t153 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t154 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t155 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t156 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t157 && EXP=0 && RUN_NO_CHECKS
SRC=t161 && COMPERR
SRC=t162 && COMPERR
SRC=t163 && COMPERR
SRC=t164 && COMPERR
SRC=t165 && COMPERR
SRC=t166 && COMPERR
SRC=t167 && COMPERR
SRC=t168 && COMPERR
SRC=t169 && COMPERR
SRC=t170 && COMPERR
SRC=t171 && COMPERR
SRC=t172 && COMPERR
SRC=t173 && COMPERR
SRC=t174 && COMPERR
# SRC=t175 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t176 && COMPERR
SRC=t177 && COMPERR
SRC=t178 && COMPERR
# SRC=t179 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t180 && COMPERR
SRC=t181 && COMPERR
SRC=t182 && COMPERR
SRC=t183 && COMPERR
SRC=t184 && COMPERR
SRC=t185 && COMPERR
SRC=t186 && COMPERR
SRC=t188 && COMPERR
SRC=t189 && COMPERR
SRC=t190 && COMPERR
SRC=t191 && COMPERR
SRC=t192 && COMPERR
SRC=t193 && COMPERR
SRC=t194 && COMPERR
SRC=t195 && COMPERR
SRC=t196 && COMPERR
SRC=t197 && COMPERR
SRC=t198 && COMPERR
SRC=t199 && COMPERR
SRC=t200 && COMPERR
SRC=t201 && COMPERR
SRC=t202 && COMPERR
SRC=t203 && COMPERR
SRC=t204 && COMPERR
SRC=t205 && COMPERR
SRC=t206 && COMPERR
SRC=t207 && COMPERR
SRC=t208 && COMPERR
SRC=t210 && COMPERR
SRC=t211 && COMPERR
SRC=t212 && COMPERR
SRC=t213 && COMPERR
SRC=t214 && COMPERR
SRC=t215 && COMPERR
SRC=t216 && COMPERR
SRC=t217 && COMPERR
SRC=t218 && COMPERR
SRC=t219 && COMPERR
SRC=t220 && COMPERR
SRC=t221 && COMPERR
# SRC=t222 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t223 && COMPERR
SRC=t224 && COMPERR
SRC=t225 && COMPERR
SRC=t226 && COMPERR
SRC=t227 && COMPERR
SRC=t228 && COMPERR
SRC=t229 && COMPERR
SRC=t230 && COMPERR
SRC=t231 && COMPERR
SRC=t232 && COMPERR
SRC=t233 && COMPERR
SRC=t234 && COMPERR
SRC=t235 && COMPERR
SRC=t236 && COMPERR
SRC=t237 && COMPERR
SRC=t238 && COMPERR
SRC=t239 && COMPERR
# SRC=t240 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t241 && COMPERR
SRC=t242 && COMPERR
SRC=t243 && COMPERR
# SRC=t245 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t246 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t249 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t248 && COMPERR
# SRC=t250 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t251 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t252 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t253 && COMPERR
SRC=t254 && COMPERR
SRC=t255 && COMPERR
SRC=t256 && COMPERR
SRC=t257 && COMPERR
SRC=t258 && COMPERR
SRC=t259 && COMPERR
SRC=t260 && COMPERR
SRC=t261 && COMPERR
# SRC=t262 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t263 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t265 && COMPERR
SRC=t266 && COMPERR
SRC=t267 && COMPERR
SRC=t269 && COMPERR
SRC=t270 && COMPERR
SRC=t271 && COMPERR
SRC=t272 && COMPERR
SRC=t273 && COMPERR
SRC=t275 && EXP=0 && RUN_NO_CHECKS
SRC=t277 && COMPERR
SRC=t278 && EXP=0 && RUN_NO_CHECKS
SRC=t279 && EXP=0 && RUN_NO_CHECKS
# SRC=t299 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t280 && COMPERR
# SRC=t286 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
# SRC=t287 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t289 && EXP=0 && RUN_NO_CHECKS
# SRC=t290 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t291 && DIFF # todo: x86 registers/syscalls; backend incomplete
# SRC=t292 && DIFF # todo: x86 registers/syscalls; backend incomplete
SRC=t293 && COMPERR
SRC=t294 && EXP=0 && RUN_NO_CHECKS
SRC=t295 && COMPERR
SRC=t296 && COMPERR
SRC=t298 && COMPERR
SRC=t300 && EXP=0 && RUN_NO_CHECKS
SRC=t301 && COMPERR
SRC=t302 && COMPERR
SRC=t303 && COMPERR
SRC=t304 && COMPERR
SRC=t307 && COMPERR
SRC=t308 && EXP=0 && RUN_NO_CHECKS
# SRC=t311 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t312 && COMPERR
# SRC=t313 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t314 && COMPERR
SRC=t315 && COMPERR
# SRC=t316 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t317 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t318 && COMPERR
SRC=t319 && COMPERR
SRC=t320 && COMPERR
# SRC=t321 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t322 && COMPERR
SRC=t323 && COMPERR
# SRC=t324 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t325 && EXP=0 && RUN_NO_CHECKS # todo: explicit i64; backend incomplete
# SRC=t326 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t327 && COMPERR
SRC=t328 && EXP=0 && RUN_NO_CHECKS
SRC=t329 && EXP=0 && RUN_NO_CHECKS
# SRC=t330 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t331 && EXP=0 && RUN_NO_CHECKS
SRC=t332 && EXP=0 && RUN_NO_CHECKS
# SRC=t333 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t334 && COMPERR
# SRC=t335 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t336 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t337 && EXP=0 && RUN_NO_CHECKS
# SRC=t338 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t339 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t340 && COMPERR
# SRC=t341 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t342 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t343 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t344 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t345 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t346 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t347 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
SRC=t348 && EXP=255 && RUN_ERR_OPTS "--vars=65536 --checks=lower,line"
SRC=t349 && EXP=255 && RUN_ERR_OPTS "--vars=65536 --checks=upper,line"
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
SRC=t375 && COMPERR
# SRC=t376 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t377 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t378 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t379 && EXP=255 && RUN_ERR # todo: RV32I runtime backend incomplete
# SRC=t380 && EXP=255 && RUN_ERR # todo: explicit i64; backend incomplete
SRC=t381 && COMPERR
SRC=t382 && COMPERR
SRC=t383 && COMPERR
# SRC=t384 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t385 && COMPERR
SRC=t386 && COMPERR
SRC=t387 && COMPERR
# SRC=t388 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t389 && COMPERR
# SRC=t390 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t391 && COMPERR
SRC=t392 && COMPERR
SRC=t393 && COMPERR
SRC=t394 && COMPERR
SRC=t395 && COMPERR
# SRC=t396 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t397 && COMPERR
SRC=t398 && COMPERR
SRC=t399 && COMPERR
# SRC=t400 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t401 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t402 && COMPERR
# SRC=t403 && EXP=0 && RUN # todo: explicit i64; backend incomplete
SRC=t404 && COMPERR
SRC=t405 && COMPERR
SRC=t406 && EXP=0 && RUN_NO_CHECKS
SRC=t407 && COMPERR
SRC=t408 && EXP=0 && RUN_NO_CHECKS
SRC=t409 && COMPERR
# SRC=t410 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t411 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t412 && EXP=0 && RUN # todo: explicit i64; backend incomplete
# SRC=t413 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t414 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t415 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t416 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
# SRC=t417 && EXP=0 && RUN # todo: RV32I runtime backend incomplete
SRC=t421 && COMPERR
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
SRC=t468 && COMPERR
SRC=t469 && COMPERR
SRC=t470 && COMPERR
# SRC=t471 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t472 && COMPERR
SRC=t473 && COMPERR
SRC=t474 && COMPERR
SRC=t475 && COMPERR
SRC=t476 && COMPERR
# SRC=t477 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t478 && COMPERR
SRC=t479 && COMPERR
# SRC=t480 && COMPERR # todo: RV32I diagnostic differs or backend is incomplete
SRC=t481 && COMPERR
SRC=t482 && EXP=42 && RUN_NO_CHECKS
SRC=t483 && EXP=42 && RUN_NO_CHECKS
# SRC=t484 && EXP=42 && RUN # todo: RV32I runtime backend incomplete
SRC=t485 && COMPERR
# SRC=t486 && DIFFINP # todo: RV32I runtime backend incomplete
SRC=t487 && COMPERR
SRC=t488 && COMPERR
