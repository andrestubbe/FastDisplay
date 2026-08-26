package fastdisplay.benchmark;

import fastdisplay.FastDisplayUtils;
import org.openjdk.jmh.annotations.*;

import java.util.concurrent.TimeUnit;

@BenchmarkMode(Mode.Throughput)
@OutputTimeUnit(TimeUnit.MILLISECONDS)
@State(Scope.Benchmark)
@Warmup(iterations = 2, time = 1, timeUnit = TimeUnit.SECONDS)
@Measurement(iterations = 3, time = 1, timeUnit = TimeUnit.SECONDS)
@Fork(1)
public class Benchmark {

    private byte[] sampleEdid;

    @Setup
    public void setup() {
        sampleEdid = new byte[128];
        // Set sample manufacturer raw code (e.g. DEL = Dell)
        sampleEdid[8] = 0x10;
        sampleEdid[9] = (byte) 0xAC;
        // Descriptor block for model name
        sampleEdid[54] = 0x00;
        sampleEdid[55] = 0x00;
        sampleEdid[56] = 0x00;
        sampleEdid[57] = (byte) 0xFC;
        byte[] modelName = "DELL U2720Q".getBytes();
        System.arraycopy(modelName, 0, sampleEdid, 59, modelName.length);
    }

    @org.openjdk.jmh.annotations.Benchmark
    public String benchmarkParseManufacturer() {
        return FastDisplayUtils.parseManufacturer(sampleEdid);
    }

    @org.openjdk.jmh.annotations.Benchmark
    public String benchmarkParseModelName() {
        return FastDisplayUtils.parseModelName(sampleEdid);
    }
}
