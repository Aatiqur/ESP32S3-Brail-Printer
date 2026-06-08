package com.example.ebrail;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.List;

// RecyclerView adapter that shows discovered devices using a CardView layout
public class DeviceAdapter extends RecyclerView.Adapter<DeviceAdapter.ViewHolder> {

    public interface Listener {
        void onConnectClicked(Device device);
        void onItemClicked(Device device);
    }

    private final List<Device> devices = new ArrayList<>();
    private final LayoutInflater inflater;
    private final Listener listener;

    public DeviceAdapter(Context context, Listener listener) {
        this.inflater = LayoutInflater.from(context);
        this.listener = listener;
    }

    public void updateDevices(List<Device> list) {
        devices.clear();
        if (list != null) devices.addAll(list);
        notifyDataSetChanged();
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View v = inflater.inflate(R.layout.item_device, parent, false);
        return new ViewHolder(v);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        Device d = devices.get(position);
        holder.name.setText(d.getName());
        holder.address.setText(d.getAddress());
        holder.rssi.setText(d.getRssi() + " dBm");
        holder.itemView.setOnClickListener(v -> {
            if (listener != null) listener.onItemClicked(d);
        });
        holder.connect.setOnClickListener(v -> {
            if (listener != null) listener.onConnectClicked(d);
        });
    }

    @Override
    public int getItemCount() {
        return devices.size();
    }

    public Device getItem(int pos) {
        return devices.get(pos);
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        TextView name, address, rssi;
        Button connect;

        ViewHolder(@NonNull View itemView) {
            super(itemView);
            name = itemView.findViewById(R.id.tv_device_name);
            address = itemView.findViewById(R.id.tv_device_address);
            rssi = itemView.findViewById(R.id.tv_device_rssi);
            connect = itemView.findViewById(R.id.btn_connect);
        }
    }
}

