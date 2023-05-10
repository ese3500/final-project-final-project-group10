import streamlit as st
import pandas as pd
import plotly.graph_objects as go


st.header("Dive Visualizer!")

@st.cache_data
def load_data():
    # data = pd.read_csv('data.csv')
    time = []
    depth = []
    with open('data.csv', 'r') as f:
        for line in f:
            if line.startswith('==FINISHED=='):
                return time,depth
            else:
                line = line.split(',')
                time.append(float(line[0]))
                depth.append(-1*float(line[1]))
    # data['Depth (m)'] = -1 * data['Depth (m)']
    return time,depth

data_load_state = st.text('Loading data...')

time,depth = load_data()

data_load_state.text('Loading data... done!')

layout_dict = {
    "title": {
        "text": "Dive Graph",
        "font": {"size": 24},
        "yanchor": "top"
    },
    "xaxis": {
        "title": {
            "text": "Time (s)",
            "font": {"size": 18}
        },
        "zeroline": False
    },
    "yaxis": {
        "title": {
            "text": "Depth (m)",
            "font": {"size": 18}
        },
        "zeroline": False
    }
}

trace_dict = {
    "type": "scatter",
    "mode": "lines+markers",
    "x": time,  
    "y": depth, 
    "hovertemplate": "Time: <b>%{x}</b> s | Depth: <b>%{y}</b> m<extra></extra>"
}


figure = go.Figure(
    data=trace_dict,
    layout=layout_dict
    )

st.plotly_chart(figure, use_container_width=True)