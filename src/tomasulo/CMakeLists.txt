create_ripes_lib(tomasulo
    LINK_TO_RIPES_LIB
    LINK_LIBS Qt6::Widgets
)
target_sources(tomasulo_lib PRIVATE
    tomasulo_config.cpp
    tomasulo_op.cpp
    tomasulo_pipeline.cpp
    tomasulo_engine.cpp
)
