@echo off

for %%f in (*.capnp) do (
     capnp compile %%f -o c++

)