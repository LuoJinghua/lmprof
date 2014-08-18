if not arg[2] then
  print("usage: " .. arg[0]
               .. " <flat | call | all> <lmprof_output_file_location> [size]")
  os.exit(1)
end

local prof_type = arg[1]
if not (prof_type == "flat" or prof_type == "call" or prof_type == "all") then
  print("invalid profile type. profile_type must be <flat | call | all>")
  os.exit(1)
end

local prof_filename = arg[2]
local ok, call_table = pcall(dofile, prof_filename)
if not ok then
  print(call_table)
  os.exit(1)
end

local size = arg[3]
local reduce = require("lmprof.reduce")
local func_table = reduce(call_table)
local lmprof_print = require("lmprof.ascii_print")

if prof_type == "all" then
  lmprof_print.flat_print(func_table, size)
  lmprof_print.call_graph_print(func_table, size)
else
  local tsize = 0
  local tf = 0
  for k,v in pairs(func_table) do
    tsize = tsize + 1
    if v.mem_perc > 1 then
      tf = tf + 1
    end
  end
  local tlimit = size or (tf > 10 and tf) or (tsize < 10 and tsize) or 10

  local header = "\n%s %s %s\n"
  local msg = "Showing %d of %d functions that allocated memory"
  local highlight = string.rep("=", (80 - string.len(msg))/2)
  msg = string.format(msg, tlimit, tsize)
  header = string.format(header, highlight, msg, highlight)
  print(header)

  if prof_type == "flat" then
    lmprof_print.flat_print(func_table, tlimit)
    print("\n" .. string.rep("=", string.len(header) - 2) .. "\n")
  elseif prof_type == "call" then
    lmprof_print.call_graph_print(func_table, tlimit)
  end
end

